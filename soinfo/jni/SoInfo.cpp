#include <stdio.h>
#include <stdlib.h>
#include <link.h>
#include <elf.h>
#include <PrintLog.h>
#include <string.h>

const char *libc_path = "/apex/com.android.runtime/bin/linker";

#define SOINFO_NAME_LEN 128

typedef struct soinfo soinfo;
struct soinfo
{
    char name[SOINFO_NAME_LEN];
    const Elf32_Phdr* phdr;
    size_t phnum;
    Elf32_Addr entry;
    Elf32_Addr base;
    unsigned size;

    uint32_t unused1;  // DO NOT USE, maintained for compatibility.

    Elf32_Dyn* dynamic;

    uint32_t unused2; // DO NOT USE, maintained for compatibility
    uint32_t unused3; // DO NOT USE, maintained for compatibility

    soinfo* next;
    unsigned flags;
};

void *GetModuleBaseAddr(pid_t pid, const char *ModuleName) {
    char szFileName[50] = {0};
    FILE *fp = NULL;
    char szMapFileLine[1024] = {0};
    char *ModulePath, *MapFileLineItem;
    long ModuleBaseAddr = 0;

    // 读取"/proc/pid/maps"可以获得该进程加载的模块
    if (pid < 0) {
        snprintf(szFileName, sizeof(szFileName), "/proc/self/maps");
    } else {
        snprintf(szFileName, sizeof(szFileName), "/proc/%d/maps", pid);
    }

    fp = fopen(szFileName, "r");
    if (fp != NULL) {
        while (fgets(szMapFileLine, sizeof(szMapFileLine), fp)) {
            if (strstr(szMapFileLine, ModuleName)) {
                MapFileLineItem = strtok(szMapFileLine, " \t");
                char *Addr = strtok(szMapFileLine, "-");
                ModuleBaseAddr = strtoul(Addr, NULL, 16);

                if (ModuleBaseAddr == 0x8000) {
                    ModuleBaseAddr = 0;
                }
                break;
            }
        }
        fclose(fp);
    }
    return (void *) ModuleBaseAddr;
}

int main(void) {
    // 获取linker模块的基地址
    void *LocalModuleAddr = GetModuleBaseAddr(-1, libc_path);

    uintptr_t LinkerAddr = (uintptr_t)LocalModuleAddr;
    uintptr_t __dl__ZL6solist = 0xCFCB0;

    void* m_soinfo_list = (void*)(LinkerAddr + __dl__ZL6solist);
    soinfo* si = reinterpret_cast<soinfo*>(*(uintptr_t*)m_soinfo_list);

    int so_count = 0;
    while (si) {
        so_count++;

        LOGD("%08x-%08x %s\n",
             si->base, si->base + si->size, si->name);

        si = si->next;
    }

    LOGD("so_count: %d", so_count);
    return 0;
}