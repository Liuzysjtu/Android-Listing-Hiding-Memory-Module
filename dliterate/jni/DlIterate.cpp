#include <stdio.h>
#include <stdlib.h>
#include <link.h>
#include <elf.h>
#include <PrintLog.h>

static int print_segment_info(struct dl_phdr_info *info, size_t size, void *data) {

    int* count = (int*)data;
    (*count)++;

    char buffer[1024];
    for (int j = 0; j < info->dlpi_phnum; j++) {
        const ElfW(Phdr) *phdr = &info->dlpi_phdr[j];
        if (phdr->p_type == PT_LOAD) { // 只关心可加载的段
            ElfW(Addr) start = info->dlpi_addr + phdr->p_vaddr;
            ElfW(Addr) end = start + phdr->p_memsz;

            // 使用 snprintf 将格式化的字符串存储到缓冲区中
            snprintf(buffer, 1024,
                     "%08lx-%08lx %c%c%c %08lx 00:00 0 %s\n",
                     start, end,
                     (phdr->p_flags & PF_R) ? 'r' : '-',
                     (phdr->p_flags & PF_W) ? 'w' : '-',
                     (phdr->p_flags & PF_X) ? 'x' : '-',
                     phdr->p_offset,
                     (info->dlpi_name && *info->dlpi_name) ? info->dlpi_name : "[anonymous]"
            );

            // 立即打印缓冲区内容
            LOGD("%s", buffer);
        }
    }
    return 0;
}

int main(void) {
    int so_count = 0;
    dl_iterate_phdr(print_segment_info, &so_count);
    LOGD("so_count = %d", so_count);
    return 0;
}