#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <bpf/bpf.h> // libbpf の API 利用のため？
#include <scx/common.h> // scx 関連．パスはおそらく scx/scheds/include/scx/
#include "scx_priority.bpf.skel.h" //eBPF スケジューラのスケルトン(BPF コードとのインタフェース)
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/file.h>

/* ----------------------------------------
 * 3 秒後に I/O イベントを発生させるスレッド
 * ---------------------------------------- */
struct writer_arg {
    int fd;
};

void *delayed_writer(void *arg)
{
    struct writer_arg *wa = arg;
    struct timespec ts = {
        .tv_sec  = 0,
        .tv_nsec = 10 * 1000 * 50  // 500 μs
    };

    nanosleep(&ts, NULL);

    write(wa->fd, "X", 1);       // POLLIN 発生
    close(wa->fd);
    //printf("  [writer] wrote data to fd\n");

    return NULL;
}

/* ---------------------------------------- */

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr,
                "Usage: %s <read_count>\n",
                argv[0]);
        return 1;
    }

    int read_count = atoi(argv[1]);
    int retry_max  = 10000;


    struct timespec timeout;
    timeout.tv_sec  = 0;
    timeout.tv_nsec = 50 * 1000;  // 50 μs

    sigset_t sigmask;
    sigemptyset(&sigmask);

    char buf[256];

    int pid = getpid();
    int tid = syscall(SYS_gettid);
    int tids_fd = bpf_obj_get("/sys/fs/bpf/priority_tids");
    int flag0 = 0;

    printf("ppoll-exp: PID=%d\n", pid);

    int ii = 0;
    
    while(ii <= 9){
        sleep(1);
        printf("%d\n", ii);
        ii++;
    }

    if (tids_fd >= 3){
         bpf_map_update_elem(tids_fd, &tid, &flag0, BPF_ANY);
    }

    for (int i = 0; i < read_count; i++) {
       // I/O リクエストを発行
       int pipefd[2];
       if (pipe(pipefd) < 0) {
           perror("pipe");
           return 1;
       }

       /* --- 3 秒後に write するスレッド生成 --- */
       pthread_t th;
       struct writer_arg wa = {
           .fd = pipefd[1]
       };

       struct pollfd pfd;
       pfd.fd = pipefd[0];
       pfd.events = POLLIN;
       //printf("[read %d] issuing ppoll()\n", i + 1);
       pthread_create(&th, NULL, delayed_writer, &wa);

        int retry = 0;
        int ret = 0;

        // ここのループに関する情報が知りたい
        do {
            ret = ppoll(&pfd, 1, &timeout, &sigmask);

            if (ret < 0) {
                perror("ppoll");
                return 1;
            }

            retry++;

        } while ((ret == 0) && (retry < retry_max));

        if (ret == 0)
            continue;

        if (pfd.revents & POLLIN) {
            ssize_t n = read(pipefd[0], buf, sizeof(buf));
            if (n > 0) {
                //printf("[read %d] event after ~3s\n", i + 1);
            }
        }
        close(pipefd[0]);
        pthread_join(th, NULL);
    }


    return 0;
}
