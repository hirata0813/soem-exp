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

static volatile sig_atomic_t stop = 0;			   

static void sigint_handler(int signo)
{
    (void)signo;
    stop = 1;   /* 終了要求フラグを立てるだけ */
}

int main(int argc, char *argv[]) {
    volatile unsigned long long sum = 0;
    int pid = getpid();
    int tid = syscall(SYS_gettid);
    int pids_fd = bpf_obj_get("/sys/fs/bpf/priority_pids");
    int tids_fd = bpf_obj_get("/sys/fs/bpf/priority_tids");
    int flag0 = 0;
    unsigned long long loop_start;
    unsigned long long loop_end;
    const unsigned long long CPU_FREQ_HZ = 3500000000UL;
    const unsigned long long threshold = 41695550000UL; // 非競合時は，これで大体1分

    ///* SIGINT ハンドラ登録 */
    //struct sigaction sa;
    //sa.sa_handler = sigint_handler;
    //sigemptyset(&sa.sa_mask);
    //sa.sa_flags = 0;
    //sigaction(SIGINT, &sa, NULL);
    
    if (pids_fd >= 3 && tids_fd >= 3){
         bpf_map_update_elem(pids_fd, &pid, &flag0, BPF_ANY);
         bpf_map_update_elem(tids_fd, &tid, &flag0, BPF_ANY);
    }

    loop_start = __rdtsc();
    while (sum <= threshold) {
        sum++;
    }
    loop_end = __rdtsc();

    printf("pid = %d, elapsed = %.9f\n", pid, (loop_end - loop_start) / (double)CPU_FREQ_HZ);

    while (1) {
        sum++;
    }

    return 0;
}
