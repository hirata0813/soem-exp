#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/file.h>
#include <bpf/bpf.h> // libbpf の API 利用のため？
#include <scx/common.h> // scx 関連．パスはおそらく scx/scheds/include/scx/
#include "scx_priority.bpf.skel.h" //eBPF スケジューラのスケルトン(BPF コードとのインタフェース)
#include <signal.h>
#include <unistd.h>
#include <x86intrin.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <poll.h>

/* eventfd */
int efd;

void logfile_output(char *fname, unsigned long long *loop_start, unsigned long long *loop_end, const unsigned long long CPU_FREQ_HZ, int cotask_num) {
  FILE *fp = fopen(fname,"a");
  if (fp == NULL) {
      perror("fopen failed");
      return;
  }
  double elapsed = 0.0;

  for (int i = 0; i < 10; i++) {
        elapsed = (loop_end[i] - loop_start[i]) / (double)CPU_FREQ_HZ;
        fprintf(fp, "%d,%.9f\n", cotask_num, elapsed);
  }
  fclose(fp);
}

/* 書き込みスレッド */
void *writer_thread(void *arg)
{
    int tids_fd = bpf_obj_get("/sys/fs/bpf/priority_tids");
    int pid = getpid();
    int tid = syscall(SYS_gettid);
    char buf[1024 * 1024];
    FILE *f = fopen("testfile", "wb");
    int flag0 = 0;
    int flag2 = 2;
    int sum = 0;

    if (tids_fd >= 3){
         bpf_map_update_elem(tids_fd, &tid, &flag2, BPF_ANY);
    }
    // 大体1sくらいかかるI/Oにする
    fwrite(buf, 1, sizeof(buf), f);
    fflush(f);	
    fsync(fileno(f));

    /* 書き込み完了を通知 */
    uint64_t one = 1;
    write(efd, &one, sizeof(one));

    fclose(f);
    close(tids_fd);
    remove("testfile");
    if (tids_fd >= 3){
         bpf_map_update_elem(tids_fd, &tid, &flag0, BPF_ANY);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    volatile unsigned long long sum = 0;
    int pid = getpid();
    int tid = syscall(SYS_gettid);
    int tids_fd = bpf_obj_get("/sys/fs/bpf/priority_tids");
    int flag0 = 0;
    int flag1 = 1;
    unsigned long long task_start;
    unsigned long long task_end;
    unsigned long long *loop_start;
    unsigned long long *loop_end;
    loop_start = (double*)malloc(sizeof(unsigned long long) * 10);
    loop_end = (double*)malloc(sizeof(unsigned long long) * 10);
    const unsigned long long CPU_FREQ_HZ = 3500000000UL;
    const unsigned long long threshold = 41100550000UL; // 非競合時は，これで大体1分
    const unsigned long long oneshot_threshold = threshold / 10;
    int cotask_num = atoi(argv[1]);
    char fname[128];
    sprintf(fname, "whole-prior-cpu-bound-task-result.csv");

    volatile unsigned long long i = 0;

    struct pollfd pfd;
    struct timespec timeout;

    timeout.tv_sec = 0;
    timeout.tv_nsec = 50 * 1000;
    

    efd = eventfd(0, 0);
    if (tids_fd >= 3){
         bpf_map_update_elem(tids_fd, &tid, &flag1, BPF_ANY);
    }
    int idx = 0;
    task_start = __rdtsc();
    while (sum <= threshold) {
        // CPU バウンド処理
        i = 0;
        while(i <= oneshot_threshold) {
            i++;
        }
        sum+=i;

        // I/O 処理(ppoll によるポーリング)
        // この区間のみ優先(競合時にここの応答性がどうなるか見る)
        int ret;
        pfd.fd = efd;
        pfd.events = POLLIN;
        pthread_t th;
        pthread_create(&th, NULL, writer_thread, NULL);
        int poll_loop = 0;

        loop_start[idx] = __rdtsc();
        do {
            unsigned long long ppoll_start = __rdtsc();
            ret = ppoll(&pfd, 1, &timeout, NULL);
            unsigned long long ppoll_end = __rdtsc();
            //printf("ppoll 処理=%.9f\n", (ppoll_end - ppoll_start) / (double)CPU_FREQ_HZ);
            poll_loop++;
        } while (ret == 0);   /* timeout → retry */

        if (ret > 0 && (pfd.revents & POLLIN)) {
            uint64_t val;
            read(efd, &val, sizeof(val));
        }
        loop_end[idx] = __rdtsc();

        /* スレッド回収 */
        pthread_join(th, NULL);
        idx++;
    }
    task_end = __rdtsc();
    printf("whole-cpu-bound-cotask-prio 全体: %.9f\n", (task_end - task_start) / (double)CPU_FREQ_HZ);

    // 各ポーリングの応答時間をファイル出力
    logfile_output(fname, loop_start, loop_end, CPU_FREQ_HZ, cotask_num);
    printf("redundant process\n");

    // 冗長処理
    while (1) {
        sum=0;
        // CPU バウンド処理
        i = 0;
        while(i <= oneshot_threshold) {
            i++;
        }
        sum+=i;

        int ret;
        pfd.fd = efd;
        pfd.events = POLLIN;
        pthread_t th;
        pthread_create(&th, NULL, writer_thread, NULL);
        int poll_loop = 0;

        if (tids_fd >= 3){
             bpf_map_update_elem(tids_fd, &tid, &flag1, BPF_ANY);
        }
        do {
            unsigned long long ppoll_start = __rdtsc();
            ret = ppoll(&pfd, 1, &timeout, NULL);
            unsigned long long ppoll_end = __rdtsc();
            //printf("ppoll 処理=%.9f\n", (ppoll_end - ppoll_start) / (double)CPU_FREQ_HZ);
            poll_loop++;
        } while (ret == 0);   /* timeout → retry */
        if (tids_fd >= 3){
             bpf_map_update_elem(tids_fd, &tid, &flag1, BPF_ANY);
        }

        if (ret > 0 && (pfd.revents & POLLIN)) {
            uint64_t val;
            read(efd, &val, sizeof(val));
        }
        /* スレッド回収 */
        pthread_join(th, NULL);
    }
    free(loop_start);
    free(loop_end);
    close(efd);

    return 0;
}
