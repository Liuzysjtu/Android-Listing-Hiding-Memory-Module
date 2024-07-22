#define _GNU_SOURCE
#include <jni.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <errno.h>
#include "PrintLog.h"

volatile sig_atomic_t signal_flag = 0;

#define SyscallCode 123456

void signal_handler(int signum, siginfo_t *siginfo, void *context) {

    signal_flag = 1;
    ucontext_t *uc = (ucontext_t *)context;
    struct sigcontext *sigc = reinterpret_cast<struct sigcontext *>(&uc->uc_mcontext);
    int code = sigc->arm_r7;
    if (code == __NR_openat) {
        char *fileName = reinterpret_cast<char *>(sigc->arm_r1);

        int fd = syscall(code, sigc->arm_r0, sigc->arm_r1, sigc->arm_r2,
                               sigc->arm_r3, sigc->arm_r4, SyscallCode);

        if (fd != -1 && strcmp(fileName, "/proc/self/maps") == 0) {
            // 创建一个临时文件用于写入过滤后的内容
            int temp_fd = syscall(__NR_memfd_create, "temp_maps", 0);
            if (temp_fd == -1) {
                // 处理错误
                close(fd);
                return;
            }
            char ch;
            char linebuf[2048]; // 假设一行不会超过2048字节
            ssize_t bytes_read;
            size_t linebuf_used = 0;

            while ((bytes_read = syscall(__NR_read, fd, &ch, 1)) == 1) {
                if (ch != '\n') {
                    if (linebuf_used < sizeof(linebuf) - 1) {
                        linebuf[linebuf_used++] = ch;
                    } else {
                        // 处理错误：行太长
                        close(fd);
                        close(temp_fd);
                        return;
                    }
                } else {
                    // 我们有一个完整的行
                    linebuf[linebuf_used] = '\0'; // Null-terminate the string
                    if (strstr(linebuf, "libdemo.so") == NULL) {
                        syscall(__NR_write, temp_fd, linebuf, linebuf_used);
                        syscall(__NR_write, temp_fd, "\n", 1);
                    }
                    linebuf_used = 0; // 重置linebuf
                }
            }
            // 写入数据到temp_fd之后
            if (lseek(temp_fd, 0, SEEK_SET) == (off_t)-1) {
                // 处理错误
                close(temp_fd);
                return;
            }
            close(fd);
            sigc->arm_r0 = temp_fd;
        } else{
            sigc->arm_r0 = fd;
        }
    }
}

void registerSignalHandler() {
    // Register signal handler
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_NODEFER | SA_ONSTACK | SA_SIGINFO;
    if (sigaction(SIGSYS, &sa, NULL) == -1) {
        LOGD("sigaction init failed.");
    }

    LOGD("Finished registering signal handler");
}

void installFilterForE() {
    struct sock_filter filter[] = {
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_openat, 0, 3),
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, args[5])),
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SyscallCode, 1, 0),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRAP),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };

    struct sock_fprog prog = {
            .len = (unsigned short) (sizeof(filter) / sizeof(filter[0])),
            .filter = filter,
    };

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
        LOGD("prctl(PR_SET_NO_NEW_PRIVS)");
    }

    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) == -1) {
        LOGD("prctl(PR_SET_SECCOMP)");
    }

    LOGD("Set new filter end");
}

void startSvcHook() {
    registerSignalHandler();
    installFilterForE();
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_demo_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    startSvcHook();

    int count = 0;

    FILE *fp = NULL;
    char szMapFilePath[32] = {0};
    char szMapFileLine[1024] = {0};
    char szPrcMapFileLine[1024] = {0};

    sprintf(szMapFilePath, "/proc/self/maps");
    LOGD("szMapFilePath: %s", szMapFilePath);


    fp = fopen(szMapFilePath, "r");
    LOGD("fp: %p", fp);
    if (fp != NULL) {
        while (fgets(szMapFileLine, 1023, fp) != NULL) {
            count++;
            LOGD("szMapFileLine: %s", szMapFileLine);
            if (strstr(szMapFileLine, "libdemo.so")) {
                memcpy(szPrcMapFileLine, szMapFileLine, strlen(szMapFileLine));
            }
        }

        fclose(fp);
    }

    LOGD("szPrcMapFileLine: %s", szPrcMapFileLine);
    LOGD("count: %d", count);

    //LOGD("flag: %d", signal_flag);

    return env->NewStringUTF(hello.c_str());
}