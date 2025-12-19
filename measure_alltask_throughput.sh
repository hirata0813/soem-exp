#!/bin/bash

# 測定用スクリプト
# pt-prio 1つと 共存タスクを同時実行し、各タスクのスループットを計測
# pt 1つと 共存タスクを同時実行し、各タスクのスループットを計測

# 設定
SIMPLE_SOEM="./pt/pt"
SCX_SOEM="./pt-prio/pt-prio"
WHOLE_SCX_SOEM="./pt-prio-whole/pt-prio-whole"
COTASK="./co-task"
ITERATIONS=1

# 共存タスクの数
cotask_count="$1"

prior_method="$2"

PRIORITY_SCHED="scx_priority"

# 出力ファイル
OUTPUT_FILE1="simple-soem-task-result.csv"
OUTPUT_FILE2="prior-soem-task-result.csv"
OUTPUT_FILE3="whole-prior-soem-task-result.csv"

echo "[INIT] Checking and stopping existing processes..."

# scheduler
if pgrep -f "scx_priority" > /dev/null; then
    echo "Stopping existing scx_priority..."
    sudo pkill -f "scx_priority"
    sleep 1
fi

# cotask
if pgrep -f "co-task" > /dev/null; then
    echo "Stopping existing cotask..."
    sudo pkill -f "co-task"
    sleep 1
fi

# SIMPLE_SOEM (pt)
if pgrep -f "./pt/pt" > /dev/null; then
    echo "Stopping existing pt (simple soem)..."
    sudo pkill -f "./pt/pt"
    sleep 1
fi

# SCX_SOEM (pt-prio)
if pgrep -f "./pt-prio/pt-prio" > /dev/null; then
    echo "Stopping existing pt-prio (scx soem)..."
    sudo pkill -f "./pt-prio/pt-prio"
    sleep 1
fi

echo "[INIT] Cleanup complete. Starting benchmark..."
echo ""

# 前回のログファイルを削除
if [ -e ${OUTPUT_FILE1} ]; then
  sudo rm ${OUTPUT_FILE1}
fi

if [ -e ${OUTPUT_FILE2} ]; then
  sudo rm ${OUTPUT_FILE2}
fi

# ==============================
# Signal handler
# ==============================
cleanup() {
    echo ""
    echo "[SIGINT] Interrupt received. Cleaning up..."

    # バックグラウンドジョブを止める
    jobs -p | while read pid; do
        if kill -0 "$pid" 2>/dev/null; then
            echo "  -> Killing background job PID: $pid"
            kill -TERM "$pid" 2>/dev/null
        fi
    done

    # scheduler
    if pgrep -f "scx_priority" > /dev/null; then
        echo "Stopping existing scx_priority..."
        sudo pkill -f "scx_priority"
        sleep 1
    fi

    # cotask
    if pgrep -f "co-task" > /dev/null; then
        echo "Stopping existing cotask..."
        sudo pkill -f "co-task"
        sleep 1
    fi

    # SIMPLE_SOEM (pt)
    if pgrep -f "./pt/pt" > /dev/null; then
        echo "Stopping existing pt (simple soem)..."
        sudo pkill -f "./pt/pt"
        sleep 1
    fi

    # SCX_SOEM (pt-prio)
    if pgrep -f "./pt-prio/pt-prio" > /dev/null; then
        echo "Stopping existing pt-prio (scx soem)..."
        sudo pkill -f "./pt-prio/pt-prio"
        sleep 1
    fi

    timestamp=$(TZ=Asia/Tokyo date +%Y%m%d-%H%M%S)

    # 優先方式によって，保存するファイル名を変える 
    if [ "$prior_method" -eq "0" ]; then
        mkdir -p "/home/hirata/soem-logs/throughput/base/${timestamp}-cotask-${cotask_count}"
        cp "$OUTPUT_FILE1" "/home/hirata/soem-logs/throughput/base/${timestamp}-cotask-${cotask_count}/$OUTPUT_FILE1"
    elif [ "$prior_method" -eq "1" ]; then
        mkdir -p "/home/hirata/soem-logs/throughput/part-prior/${timestamp}-cotask-${cotask_count}"
        cp "$OUTPUT_FILE2" "/home/hirata/soem-logs/throughput/part-prior/${timestamp}-cotask-${cotask_count}/$OUTPUT_FILE2"
    elif [ "$prior_method" -eq "2" ]; then
    	mkdir -p "/home/hirata/soem-logs/throughput/whole-prior/nice/sched_other/${timestamp}-cotask-${cotask_count}"
    	cp "$OUTPUT_FILE1" "/home/hirata/soem-logs/throughput/whole-prior/nice/sched_other/${timestamp}-cotask-${cotask_count}/$OUTPUT_FILE1"
    elif [ "$prior_method" -eq "3" ]; then
    	mkdir -p "/home/hirata/soem-logs/throughput/whole-prior/sched_fifo/${timestamp}-cotask-${cotask_count}"
    	cp "$OUTPUT_FILE1" "/home/hirata/soem-logs/throughput/whole-prior/sched_fifo/${timestamp}-cotask-${cotask_count}/$OUTPUT_FILE1"
    elif [ "$prior_method" -eq "4" ]; then
    	mkdir -p "/home/hirata/soem-logs/throughput/whole-prior/scx_whole/${timestamp}-cotask-${cotask_count}"
    	cp "$OUTPUT_FILE3" "/home/hirata/soem-logs/throughput/whole-prior/scx_whole/${timestamp}-cotask-${cotask_count}/$OUTPUT_FILE3"
    else
    	echo "Copy Error!!"
    fi

    echo "[SIGINT] Cleanup completed. Exiting."
    exit 130   # SIGINT の標準終了コード
}

# SIGINT (Ctrl+C) と SIGTERM を捕捉
trap cleanup SIGINT SIGTERM


# scx_priorityが動いているか確認
check_scheduler() {
    local ts_multi=$1
    local num_dispatch=$2
    if ! pgrep -f "scx_priority" > /dev/null; then
      echo "Starting priority scheduler..."
      sudo $PRIORITY_SCHED $ts_multi $num_dispatch &
  
      # スケジューラが起動するまで待つ
      sleep 2
  
    fi

    echo "scx_priority is running. Proceeding with benchmark..."
}

stop_scheduler() {
    if pgrep -f "scx_priority" > /dev/null; then
        echo "Stopping priority scheduler..."
        sudo pkill -f "scx_priority"
        sleep 2
    else
        echo "scx_priority is not running."
    fi
}



# メイン測定ループ
run_benchmark() {
    local slice_mult=$1
    local dispatch_limit=$2
    local cotask_count=$3
    local iteration=$4
    local benchmark=$5
    
    echo ""
    echo "Running iteration $iteration with $cotask_count cotasks"
    
    declare -a pids

    if [ "$prior_method" -eq "0" ]; then
    	# 計測1(ベース実行時間)

        # 共存タスク開始
        for ((i=1; i<=$cotask_count; i++)); do
            $COTASK &
            cotask_pid=$!
            echo "  -> cotask $i PID: $cotask_pid"
            pids+=($cotask_pid)
	        sleep 0.1
        done
        sleep 0.1
        # SOEM タスク開始
        sudo $benchmark 10000 $cotask_count &
        soem_pid=$!
        echo "  -> soem PID: $soem_pid"
        pids+=($soem_pid)

        # 配列の要素数を取得
        pids_length=${#pids[@]}

        # 全てのタスクの完了を待つ
        echo "Waiting for all tasks to complete..."
        for pid in "${pids[@]}"; do
            wait $pid 2>/dev/null
        done
    elif [ "$prior_method" -eq "1" ]; then
    	# 計測2(部分優先実行)

        # 共存タスク開始
        for ((i=1; i<=$cotask_count; i++)); do
            $COTASK &
            cotask_pid=$!
            echo "  -> cotask $i PID: $cotask_pid"
            pids+=($cotask_pid)
	        sleep 0.1
        done
        sleep 0.1
        # SOEM タスク開始
        sudo $benchmark 10000 $cotask_count &
        soem_pid=$!
        echo "  -> soem PID: $soem_pid"
        pids+=($soem_pid)

        # 配列の要素数を取得
        pids_length=${#pids[@]}

        # 全てのタスクの完了を待つ
        echo "Waiting for all tasks to complete..."
        for pid in "${pids[@]}"; do
            wait $pid 2>/dev/null
        done
    elif [ "$prior_method" -eq "2" ]; then
    	# 計測3(nice)

        # 共存タスク開始
        for ((i=1; i<=$cotask_count; i++)); do
            taskset -c 0 $COTASK &
            cotask_pid=$!
            echo "  -> cotask $i PID: $cotask_pid"
            pids+=($cotask_pid)
	        sleep 0.1
        done
        sleep 0.1
        # SOEM タスク開始
        sudo taskset -c 0 nice -n -20 $benchmark 10000 $cotask_count &
        soem_pid=$!
        echo "  -> soem PID: $soem_pid"
        pids+=($soem_pid)

        # 配列の要素数を取得
        pids_length=${#pids[@]}

        # 全てのタスクの完了を待つ
        echo "Waiting for all tasks to complete..."
        for pid in "${pids[@]}"; do
            wait $pid 2>/dev/null
        done
    elif [ "$prior_method" -eq "3" ]; then
    	# 計測4(SCHED_FIFO)

        # 共存タスク開始
        for ((i=1; i<=$cotask_count; i++)); do
            taskset -c 0 $COTASK &
            cotask_pid=$!
            echo "  -> cotask $i PID: $cotask_pid"
            pids+=($cotask_pid)
	        sleep 0.1
        done
        sleep 0.1
        # SOEM タスク開始
        sudo taskset -c 0 chrt -f 99 $benchmark 10000 $cotask_count &
        soem_pid=$!
        echo "  -> soem PID: $soem_pid"
        pids+=($soem_pid)

        # 配列の要素数を取得
        pids_length=${#pids[@]}

        # 全てのタスクの完了を待つ
        echo "Waiting for all tasks to complete..."
        for pid in "${pids[@]}"; do
            wait $pid 2>/dev/null
        done
    elif [ "$prior_method" -eq "4" ]; then
    	# 計測5(sched_ext 最初から最後まで)

        # 共存タスク開始
        for ((i=1; i<=$cotask_count; i++)); do
            $COTASK &
            cotask_pid=$!
            echo "  -> cotask $i PID: $cotask_pid"
            pids+=($cotask_pid)
	        sleep 0.1
        done
        sleep 0.1
        # SOEM タスク開始
        sudo perf stat $benchmark 10000 $cotask_count &
        soem_pid=$!
        echo "  -> soem PID: $soem_pid"
        pids+=($soem_pid)

        # 配列の要素数を取得
        pids_length=${#pids[@]}

        # 全てのタスクの完了を待つ
        echo "Waiting for all tasks to complete..."
        for pid in "${pids[@]}"; do
            wait $pid 2>/dev/null
        done
    else
    	echo "Error!!"
    fi

    echo "All tasks completed for iteration $iteration with $cotask_count cotasks"
}

# メイン実行
main() {
    echo "Starting benchmark with scx_priority scheduler"

    # パラメータ配列の定義
    #cotask_counts=(0 4 8)           # cotask の数
    joined=$(IFS=-; echo "${cotask_count}") # 出力ファイルの名前用

    timestamp=$(TZ=Asia/Tokyo date +%Y%m%d-%H%M%S)

    echo "cotask_num,elapsed" > $OUTPUT_FILE1
    echo "cotask_num,elapsed" > $OUTPUT_FILE2
    echo "cotask_num,elapsed" > $OUTPUT_FILE3

    if [ "$prior_method" -eq "0" ]; then
    	# 計測1(ベース実行時間)
    	echo ""
    	echo "=============================================="
    	echo "Measure 1: ベース実行時間(優先処理なし)"
    	echo "=============================================="
    	
    	echo "Testing with ${cotask_count} cotasks"

    	# 各イテレーション(1~10)について測定
    	for ((iteration=1; iteration<=ITERATIONS; iteration++)); do
    	    check_scheduler 1 1
    	    run_benchmark 1 1 $cotask_count $iteration $SIMPLE_SOEM
    	    # スケジーラの停止
    	    stop_scheduler
    	done
    	

    	mkdir -p "/home/hirata/soem-logs/simple-soem/${timestamp}-cotask-${joined}"
    	cp "$OUTPUT_FILE1" "/home/hirata/soem-logs/simple-soem/${timestamp}-cotask-${joined}/$OUTPUT_FILE1"
    elif [ "$prior_method" -eq "1" ]; then
    	# 計測2(部分優先実行)
    	echo ""
    	echo "=============================================="
    	echo "Measure 2: 部分優先実行"
    	echo "=============================================="
    	
    	echo "Testing with ${cotask_count} cotasks"

    	# 各イテレーション(1~10)について測定
    	for ((iteration=1; iteration<=ITERATIONS; iteration++)); do
    	    check_scheduler 1 1
    	    run_benchmark 1 1 $cotask_count $iteration $SCX_SOEM
    	    # スケジーラの停止
    	    stop_scheduler
    	done
    	
    	mkdir -p "/home/hirata/soem-logs/scx-soem/${timestamp}-cotask-${joined}"
    	cp "$OUTPUT_FILE2" "/home/hirata/soem-logs/scx-soem/${timestamp}-cotask-${joined}/$OUTPUT_FILE2"
    elif [ "$prior_method" -eq "2" ]; then
    	# 計測3(nice)
    	echo ""
    	echo "=============================================="
    	echo "Measure 3: 全体優先実行(nice コマンド)"
    	echo "=============================================="
    	
    	echo "Testing with ${cotask_count} cotasks"

    	# 各イテレーション(1~10)について測定
    	for ((iteration=1; iteration<=ITERATIONS; iteration++)); do
    	    #check_scheduler 1 1
    	    run_benchmark 1 1 $cotask_count $iteration $SIMPLE_SOEM
    	    # スケジーラの停止
    	    stop_scheduler
    	done
    	

    	mkdir -p "/home/hirata/soem-logs/simple-soem/nice/${timestamp}-cotask-${joined}"
    	cp "$OUTPUT_FILE1" "/home/hirata/soem-logs/simple-soem/nice/${timestamp}-cotask-${joined}/$OUTPUT_FILE1"
    elif [ "$prior_method" -eq "3" ]; then
    	# 計測4(SCHED_FIFO)
    	echo ""
    	echo "=============================================="
    	echo "Measure 4: 全体優先実行(SCHED_FIFO)"
    	echo "=============================================="
    	
    	echo "Testing with ${cotask_count} cotasks"

    	# 各イテレーション(1~10)について測定
    	for ((iteration=1; iteration<=ITERATIONS; iteration++)); do
    	    run_benchmark 1 1 $cotask_count $iteration $SIMPLE_SOEM
    	    # スケジーラの停止
    	    stop_scheduler
    	done
    	

    	mkdir -p "/home/hirata/soem-logs/simple-soem/sched_fifo/${timestamp}-cotask-${joined}"
    	cp "$OUTPUT_FILE1" "/home/hirata/soem-logs/simple-soem/sched_fifo/${timestamp}-cotask-${joined}/$OUTPUT_FILE1"
    elif [ "$prior_method" -eq "4" ]; then
    	# 計測5(sched_ext 最初から最後まで)
    	echo ""
    	echo "=============================================="
    	echo "Measure 5: 全体優先実行(sched_ext 最初から最後まで)"
    	echo "=============================================="
    	
    	echo "Testing with ${cotask_count} cotasks"

    	# 各イテレーション(1~10)について測定
    	for ((iteration=1; iteration<=ITERATIONS; iteration++)); do
    	    check_scheduler 1 1
    	    run_benchmark 1 1 $cotask_count $iteration $WHOLE_SCX_SOEM
    	    # スケジーラの停止
    	    stop_scheduler
    	done

    	mkdir -p "/home/hirata/soem-logs/simple-soem/scx_whole/${timestamp}-cotask-${joined}"
    	cp "$OUTPUT_FILE3" "/home/hirata/soem-logs/simple-soem/scx_whole/${timestamp}-cotask-${joined}/$OUTPUT_FILE3"
    else
    	echo "Error!!"
    fi

    echo "Finished."
}

# 実行
main "$@"
