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

# ---- bpftrace スクリプト出力 ---------------
cat <<EOF
config = {
    max_map_keys=65536
}

BEGIN {
    @idx = 0;
}

tracepoint:sched:sched_switch
{
    if ($cond) {
        @ts[@idx]   = nsecs;
        @next[@idx] = args->next_pid;
        @prev[@idx] = args->prev_pid;
	@cpu[@idx] = cpu;
        @idx++;
    }
}
EOF
