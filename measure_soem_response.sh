#!/bin/bash

# 測定用スクリプト
# pt-prio 1つと 外乱を同時実行し、実行時間を計測
# pt 1つと 外乱を同時実行し、実行時間を計測

# 設定
SIMPLE_SOEM="./pt/pt"
SCX_SOEM="./pt-prio/pt-prio"
DISTURB="./infinityloop"
ITERATIONS=1

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

# disturb（infinityloop）
if pgrep -f "infinityloop" > /dev/null; then
    echo "Stopping existing infinityloop tasks..."
    sudo pkill -f "infinityloop"
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
    local disturb_count=$3
    local iteration=$4
    local benchmark=$5
    
    echo ""
    echo "Running iteration $iteration with $disturb_count disturb tasks"
    
    declare -a pids
    declare -a disturb_pids

    # disturb 開始（CPUバウンドなバックグラウンドタスク）
    for ((i=1; i<=$disturb_count; i++)); do
        $DISTURB &
        infinity_pid=$!
        echo "  -> disturb $i PID: $infinity_pid"
        disturb_pids+=($infinity_pid)
	sleep 0.1
    done

    sleep 3

    
    # SOEM タスク開始
    sudo $benchmark 10000 $disturb_count &
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
    
    echo "All tasks completed for iteration $iteration with $disturb_count disturbs"


    # disturb タスクを強制終了
    echo "Terminating remaining disturbs..."
    for disturb_pid in "${disturb_pids[@]}"; do
        if kill -0 "$disturb_pid" 2>/dev/null; then
            echo "Killing remaining task (PID: $disturb_pid)"
            kill -TERM "$disturb_pid" 2>/dev/null
        fi
    done
    
}

# メイン実行
main() {
    echo "Starting benchmark with scx_priority scheduler"

    # パラメータ配列の定義
    slice_multipliers=(1)         # タイムスライスの倍率
    dispatch_limits=(1)         # ディスパッチ制限数（-1は無制限）
    disturb_counts=(12 16)           # infinity_loopの数
    joined=$(IFS=-; echo "${disturb_counts[*]}") # 出力ファイルの名前用


    echo "disturb_num,elapsed" > $OUTPUT_FILE1
    echo "disturb_num,elapsed" > $OUTPUT_FILE2

    # 計測1(SIMPLE_SOEM)
    for dispatch_limit in "${dispatch_limits[@]}"; do
        echo ""
        echo "=============================================="
            echo "Measure 1: SIMPLE SOEM"
        echo "=============================================="

    	for slice_mult in "${slice_multipliers[@]}"; do
            echo ""
            echo "--- Testing with slice multiplier: ${slice_mult} ---"
    
            for disturb_count in "${disturb_counts[@]}"; do
                echo ""
                echo "Testing with ${disturb_count} disturbs"

                # 各イテレーション(1~10)について測定
        	    for ((iteration=1; iteration<=ITERATIONS; iteration++)); do
            		check_scheduler $slice_mult $dispatch_limit
                    run_benchmark $slice_mult $dispatch_limit $disturb_count $iteration $SIMPLE_SOEM
            		# スケジーラの停止
            		stop_scheduler
                done
            done
        done
    done
    
    timestamp=$(TZ=Asia/Tokyo date +%Y%m%d-%H%M%S)

    mkdir -p "/home/hirata/soem-logs/simple-soem/${timestamp}-dis-${joined}"
    cp "$OUTPUT_FILE1" "/home/hirata/soem-logs/simple-soem/${timestamp}-dis-${joined}/$OUTPUT_FILE1"

    # 計測2(SCX SOEM)
    for dispatch_limit in "${dispatch_limits[@]}"; do
        echo ""
        echo "=============================================="
	    echo "Measure 2: SCX SOEM"
        echo "=============================================="

    	for slice_mult in "${slice_multipliers[@]}"; do
            echo ""
            echo "--- Testing with slice multiplier: ${slice_mult} ---"
    
            for disturb_count in "${disturb_counts[@]}"; do
                echo ""
                echo "Testing with ${disturb_count} disturbs"

                # 各イテレーション(1~10)について測定
        	    for ((iteration=1; iteration<=ITERATIONS; iteration++)); do
            		check_scheduler $slice_mult $dispatch_limit
                    run_benchmark $slice_mult $dispatch_limit $disturb_count $iteration $SCX_SOEM
            		# スケジーラの停止
            		stop_scheduler
                done
            done
        done
    done
    
    timestamp=$(TZ=Asia/Tokyo date +%Y%m%d-%H%M%S)

    mkdir -p "/home/hirata/soem-logs/scx-soem/${timestamp}-dis-${joined}"
    cp "$OUTPUT_FILE2" "/home/hirata/soem-logs/scx-soem/${timestamp}-dis-${joined}/$OUTPUT_FILE2"

    echo "Finished."
}

# 実行
main "$@"
