#!/bin/bash

# 使用例:
#   ./gen_bpftrace.sh myprog anotherprog

if [ $# -eq 0 ]; then
    echo "Usage: $0 process_name1 [process_name2 ...]"
    exit 1
fi

# ---- PID 取得 -----------------------------
PIDS=()
for pname in "$@"; do
    # 該当プロセスの PID をすべて取得
    for pid in $(pgrep -x "$pname"); do
        PIDS+=("$pid")
    done
done

if [ ${#PIDS[@]} -eq 0 ]; then
    echo "Error: No matching processes found."
    exit 1
fi

# ---- if 条件生成 ---------------------------
cond=""
for pid in "${PIDS[@]}"; do
    if [ -n "$cond" ]; then
        cond="$cond || args->next_pid == $pid || args->prev_pid == $pid"
    else
        cond="args->next_pid == $pid || args->prev_pid == $pid"
    fi
done

cond2=""
for pid in "${PIDS[@]}"; do
    if [ -n "$cond2" ]; then
        cond2="$cond2 || args->pid == $pid"
    else
        cond2="args->pid == $pid"
    fi
done

# ---- bpftrace スクリプト出力 ---------------
cat <<EOF
BEGIN{
    @start_time[0] = nsecs;
}

tracepoint:sched:sched_switch
/ cpu == 0 /
{
    if ($cond) {
        /*
         * prev_state:
         * 0        = TASK_RUNNING (runnable)
         * 1        = TASK_INTERRUPTIBLE
         * 2        = TASK_UNINTERRUPTIBLE
         */

        \$ts = nsecs;

        if (args->prev_state == 0) {
            printf("[%llu.%06llu] PID:%d RUNNING → RUNNABLE(停止), PID:%d RUNNABLE → RUNNING(開始)\n",
                (\$ts - @start_time[0]) / 1000000000,
                ((\$ts - @start_time[0]) % 1000000000) / 1000,
                args->prev_pid, args->next_pid);

        } else {
            printf("[%llu.%06llu] PID:%d RUNNING → SLEEP(停止), PID:%d RUNNABLE → RUNNING(開始)\n",
                (\$ts - @start_time[0]) / 1000000000,
                ((\$ts - @start_time[0]) % 1000000000) / 1000,
                args->prev_pid, args->next_pid);
        }
    }
}

tracepoint:sched:sched_wakeup
/ cpu == 0 /
{
    if ($cond2) {
        \$ts = nsecs;
        printf("[%llu.%06llu] PID:%d WAKEUP\n",
               (\$ts - @start_time[0]) / 1000000000,
               ((\$ts - @start_time[0]) % 1000000000) / 1000,
               args->pid);
    }
}
EOF
