#!/usr/bin/env python3
import sys
import matplotlib.pyplot as plt
import os
import japanize_matplotlib

MAX_RTT = 10000        # RTT の最大値(us)
BIN_US_WIDTH = 10      # ヒストグラムのビン幅(us)
OVER_THRESHOD_US = 500 # RTT の警告閾値(us)
TARGET_COL = 2         # 0始まり → 3列目を対象にする

def main():
    if len(sys.argv) != 2:
        print(f"Usage: python {sys.argv[0]} <RTT data file>")
        sys.exit(1)

    filepath = sys.argv[1]
    rtt_list = []

    # ログファイル形式: 各行に複数列の値 (例: 100.01,200.02,300.03)
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if line:  # 空行は無視
                try:
                    parts = line.split(",")
                    if len(parts) > TARGET_COL:
                        rtt_list.append(float(parts[TARGET_COL]))
                    else:
                        print(f"[WARN] 列数不足でスキップ: {line}")
                except ValueError:
                    print(f"[WARN] 数値変換できない行をスキップ: {line}")

    if not rtt_list:
        print("No RTT data found.")
        sys.exit(1)

    # 出力ファイル名を入力ファイル名ベースで作成
    base = os.path.splitext(os.path.basename(filepath))[0]
    save_filename = base + '_rtt_hist.png'

    mean_rtt = sum(rtt_list) / len(rtt_list)
    max_rtt = max(rtt_list)
    min_rtt = min(rtt_list)
    print(f"平均 RTT: {mean_rtt:.2f} us\n最大 RTT: {max_rtt:.2f} us\n")

    # ビンの作成
    bins = list(range(0, int(MAX_RTT + BIN_US_WIDTH), int(BIN_US_WIDTH)))

    plt.figure(figsize=(10, 6))
    n, bins_edges, patches = plt.hist(
        rtt_list, bins=bins, color="blue", log=True
    )

    # 平均線
    plt.axvline(mean_rtt, color='green', linestyle='dashed', linewidth=2,
                label=f'平均 RTT: {mean_rtt:.2f} us')
    plt.plot([], [], ' ', label=f'最大 RTT: {max_rtt:.2f} us')

    # 閾値超えの RTT を表示
    print(f"[INFO] RTT > {OVER_THRESHOD_US} us")
    rtt_over = [rtt for rtt in rtt_list if rtt > OVER_THRESHOD_US]
    for rtt in rtt_over:
        print(f"{rtt:.2f} us")
    print(f"RTT > {OVER_THRESHOD_US} us count: {len(rtt_over)} / ({len(rtt_list)})")

    # y 軸を対数スケール、freq=1 を必ず表示
    plt.yscale('log')
    plt.ylim(0.8, max(n) * 1.2 if len(n) > 0 else 10)

    # x 軸範囲を最大 RTT に合わせて調整
    plt.xlim(0, ((max_rtt // 1000) + 1) * 1000)

    plt.title(f'ラウンドトリップ の処理時間ヒストグラム '
              f'({BIN_US_WIDTH}us 間隔, 通信回数: {len(rtt_list)})')
    plt.xlabel('処理時間 (us)')
    plt.ylabel('頻度 (log scale)')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()

    plt.savefig(save_filename)
    print(f"Saved histogram plot as {save_filename}")

if __name__ == "__main__":
    main()
