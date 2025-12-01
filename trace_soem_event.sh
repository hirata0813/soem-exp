#!/bin/bash

# SOEM のイベントをトレースするスクリプト
# まず，measure_soem_response.sh を起動(外乱の数，pt/pt-prioを指定)
# その後，bpftrace コードをgenerate
# その後，bpftrace を実行(ログファイル名には，外乱の数，SOEMのPIDを含める)
# その後，measure_soem_response.sh の終了を待つ
# measure_soem_response.shが終了したら，bpftrace にシグナルを送る
# 上記を，外乱数0,4,8,12で実験


# 出力ファイル
OUTPUT_FILE1="simple-soem-task-result.csv"
OUTPUT_FILE2="prior-soem-task-result.csv"

TARGET_CMD="./measure_soem_response.sh"        # トレース対象コマンド
BPF_FILE="trace.bt"          # bpftrace スクリプトファイル（例: trace.bt）

if [ -z "$TARGET_CMD" ] || [ -z "$BPF_FILE" ]; then
    echo "Usage: $0 <target-command> <bpftrace-file>"
    exit 1
fi

disturb_counts=(0 4 8 12)
for disturb_count in "${disturb_counts[@]}"; do
    echo ""
    echo "Tracing pt with ${disturb_count} disturbs"
    # ---- トレース対象プログラムをバックグラウンド起動 ----
    sudo $TARGET_CMD $disturb_count 0 &
    TARGET_PID=$!

    sleep 10
    
    # ---- bpftrace コードを generate ----
    ./gen-bpftrace.sh pt pt-prio infinityloop > "$BPF_FILE"

    SOEM_PID=$(pgrep -x "pt")

    LOGFILE="pt-${disturb_count}.log"
    
    # ---- bpftrace をバックグラウンド起動 ----
    sudo bpftrace "$BPF_FILE" > "$LOGFILE" &
    BPFT_PID=$!
    
    echo "bpftrace PID: $BPFT_PID"
    
    # ---- ターゲット終了待ち ----
    wait $TARGET_PID
    
    echo "Target finished. Sending SIGINT to bpftrace..."
    
    # ---- bpftrace に Ctrl-C 相当を送る ----
    kill -INT $BPFT_PID
    
    # ---- bpftrace のプロセス終了を待つ ----
    wait $BPFT_PID
    echo "bpftrace finished."

    # ---- bpftrace のコードをバックアップ ----
    mkdir -p "/home/hirata/soem-trace/pt-${disturb_count}"
    cp "$LOGFILE" "/home/hirata/soem-trace/pt-${disturb_count}/$LOGFILE"
    cp "$BPFFILE" "/home/hirata/soem-trace/pt-${disturb_count}/$BPFFILE"
done

for disturb_count in "${disturb_counts[@]}"; do
    echo ""
    echo "Tracing pt-prio with ${disturb_count} disturbs"
    # ---- トレース対象プログラムをバックグラウンド起動 ----
    $TARGET_CMD $disturb_count 1 &
    TARGET_PID=$!

    sleep 10
    
    # ---- bpftrace コードを generate ----
    ./gen-bpftrace.sh pt pt-prio infinityloop > "$BPF_FILE"

    SOEM_PID=$(pgrep -x "pt-prio")

    LOGFILE="ptprio-${disturb_count}-${SOEM_PID}.log"
    
    # ---- bpftrace をバックグラウンド起動 ----
    sudo bpftrace "$BPF_FILE" > "$LOGFILE" &
    BPFT_PID=$!
    
    echo "bpftrace PID: $BPFT_PID"
    
    # ---- ターゲット終了待ち ----
    wait $TARGET_PID
    
    echo "Target finished. Sending SIGINT to bpftrace..."
    
    # ---- bpftrace に Ctrl-C 相当を送る ----
    kill -INT $BPFT_PID
    
    # ---- bpftrace のプロセス終了を待つ ----
    wait $BPFT_PID
    echo "bpftrace finished."

    # ---- bpftrace のコードをバックアップ ----
    mkdir -p "/home/hirata/soem-trace/ptprio-${disturb_count}"
    cp "$LOGFILE" "/home/hirata/soem-trace/ptprio-${disturb_count}/$LOGFILE"
    cp "$BPFFILE" "/home/hirata/soem-trace/ptprio-${disturb_count}/$BPFFILE"
done
