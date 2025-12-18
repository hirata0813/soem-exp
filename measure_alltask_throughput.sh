#!/bin/bash

# 測定用スクリプト
# pt-prio 1つと 共存タスクを同時実行し、各タスクのスループットを計測
# pt 1つと 共存タスクを同時実行し、各タスクのスループットを計測

# 設定
SIMPLE_SOEM="./pt/pt"
SCX_SOEM="./pt-prio/pt-prio"
COTASK="./co-task"
ITERATIONS=1

# 外乱の数
cotask_count="$1"

PRIORITY_SCHED="scx_priority"

# 出力ファイル
OUTPUT_FILE1="simple-soem-task-result.csv"
OUTPUT_FILE2="prior-soem-task-result.csv"

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

    # 共存 開始（CPUバウンドなバックグラウンドタスク）
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
    
    echo "All tasks completed for iteration $iteration with $cotask_count cotasks"


    # 共存タスクを強制終了
    #echo "Terminating remaining cotasks..."
    #for disturb_pid in "${disturb_pids[@]}"; do
    #    if kill -0 "$disturb_pid" 2>/dev/null; then
    #        echo "Killing remaining task (PID: $disturb_pid)"
    #        kill -TERM "$disturb_pid" 2>/dev/null
    #    fi
    #done
    
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

    if [ "$2" -eq "0" ]; then
    	# 計測1(SIMPLE_SOEM)
    	echo ""
    	echo "=============================================="
    	echo "Measure 1: SIMPLE SOEM"
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
    elif [ "$2" -eq "1" ]; then
    	# 計測2(SCX_SOEM)
    	echo ""
    	echo "=============================================="
    	echo "Measure 2: SCX SOEM"
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
    elif [ "$2" -eq "2" ]; then
    	# 計測1(SIMPLE_SOEM)
    	echo ""
    	echo "=============================================="
    	echo "Measure 1: SIMPLE SOEM"
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

    	# 計測2(SCX_SOEM)
    	echo ""
    	echo "=============================================="
    	echo "Measure 2: SCX SOEM"
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
    else
    	echo "Error!!"
    fi

    echo "Finished."
}

# 実行
main "$@"
