import sys
import matplotlib.pyplot as plt
import os
import japanize_matplotlib

MAX_RTT = 10000  # Threshold for highlighting RTT values
BIN_US_WIDTH = 10  # Width of histogram bins in microseconds
OVER_THRESHOD_US = 500

def main():
    if len(sys.argv) != 2:
        print(f"Usage: python {sys.argv[0]} <RTT data file>")
        sys.exit(1)

    filepath = sys.argv[1]
    rtt_list = []

    if "soem" in filepath:
        with open(filepath, 'r') as f:
            pt_list = [line.strip().split(',') for line in f]
            rtt_list = [float(rtt[3]) for rtt in pt_list if len(rtt) > 3]
    else:
        with open(filepath, 'r') as f:
            rtt_list = [float(line.strip()) for line in f if line.strip()]

    if not rtt_list:
        print("No RTT data found.")
        sys.exit(1)

    # 入力ファイルの拡張子取得
    base = os.path.splitext(os.path.basename(filepath))[0]
    save_filename = base + '_rtt_hist.png'

    mean_rtt = sum(rtt_list) / len(rtt_list)
    max_rtt = max(rtt_list)
    min_rtt = min(rtt_list)
    print(f"平均 RTT: {mean_rtt:.2f} us\n最大 RTT: {max_rtt:.2f} us\n")

    # bins の作成
    bins = [i for i in range(0, int(MAX_RTT + BIN_US_WIDTH), int(BIN_US_WIDTH))]

    plt.figure(figsize=(10, 6))
    n, bins_edges, patches = plt.hist(
        rtt_list, bins=bins, color="blue", log=True
    )

    # print("counts =", n.astype(int))  # デバッグ出力（numpy なしで int に）

    # 平均線
    plt.axvline(mean_rtt, color='green', linestyle='dashed', linewidth=2,
                label=f'平均 RTT: {mean_rtt:.2f} us')
    plt.plot([], [], ' ', label=f'最大 RTT: {max_rtt:.2f} us')

    print(f"[INFO] RTT > {OVER_THRESHOD_US} us")
    rtt_over = [rtt for rtt in rtt_list if rtt > OVER_THRESHOD_US]
    for rtt in rtt_over:
        print(f"{rtt:.2f} us")
    print(f"RTT > {OVER_THRESHOD_US} us count: {len(rtt_over)} / ({len(rtt_list)})")

    # y 軸対数で freq=1 を必ず表示させる
    plt.yscale('log')
    # plt.xscale('log')
    plt.ylim(0.8, max(n) * 1.2 if len(n) > 0 else 10)

    plt.xlim(0, ((max_rtt // 1000) + 1) * 1000)
    plt.title(f'ラウンドトリップ(SOEM send to recv) の処理時間ヒストグラム '
              f'({BIN_US_WIDTH}us 間隔, 通信回数: {len(rtt_list)})')
    plt.xlabel('処理時間 (us)')
    plt.ylabel('頻度 (log scale)')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()

    plt.savefig(save_filename)
    print(f"Saved histogram plot as {save_filename}")
    # print([rtt for rtt in rtt_list if rtt > 4000.0])

if __name__ == "__main__":
    main()
