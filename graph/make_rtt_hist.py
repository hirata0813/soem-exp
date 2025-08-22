import sys
import matplotlib.pyplot as plt
import numpy as np
import os

MAX_RTT = 500  # Threshold for highlighting RTT values
BIN_US_WIDTH = 5  # Width of histogram bins in microseconds

def main():
    if len(sys.argv) != 2:
        print(f"Usage: python {sys.argv[0]} <RTT data file>")
        sys.exit(1)

    filepath = sys.argv[1]

    try:
        with open(filepath, 'r') as f:
            rtt_list = [float(line.strip()) for line in f if line.strip()]
    except Exception as e:
        print(f"Error reading file: {e}")
        sys.exit(1)

    if not rtt_list:
        print("No RTT data found.")
        sys.exit(1)

  # 入力ファイルの拡張子取得
    base = os.path.splitext(os.path.basename(filepath))[0]

    # 保存ファイル名を入力ファイル名 + '.png'にする例
    save_filename = base + '.png'


    mean_rtt = np.mean(rtt_list)
    max_rtt = max(rtt_list)
    min_rtt = min(rtt_list)
    print(f"Mean RTT: {mean_rtt:.2f} us\nMax RTT: {max_rtt:.2f} us\nMin RTT: {min_rtt:.2f} us")

    # Histogram bin setup (bin width = 5)
    bins = np.arange(0, MAX_RTT + BIN_US_WIDTH, BIN_US_WIDTH)
    counts, edges = np.histogram(rtt_list, bins=bins)

    # Bar colors: highlight values over 200
    bar_colors = ['skyblue' if edge <= 200 else 'tomato' for edge in edges[:-1]]

    plt.figure(figsize=(10, 6))
    plt.bar(edges[:-1], counts, width=np.diff(edges), align='edge',
            color=bar_colors, edgecolor='black')

    # Mean RTT line
    plt.axvline(mean_rtt, color='green', linestyle='dashed', linewidth=2,
                label=f'Mean RTT: {mean_rtt:.2f} us')
    # ダミー線で凡例に統計情報だけ表示
    plt.plot([], [], ' ', label=f'Min RTT: {min_rtt:.2f} us')
    plt.plot([], [], ' ', label=f'Max RTT: {max_rtt:.2f} us')

    print("[INFO] RTT > 200 us")
    # Vertical lines for RTT > 200
    rtt_over_200 = [rtt for rtt in rtt_list if rtt > 200]
    shown_legend = False
    for rtt in rtt_over_200:
        print(f"{rtt:.2f} us")
        if not shown_legend:
            plt.axvline(rtt, color='red', alpha=0.3, linewidth=1, linestyle='-', label='RTT > 200 us')
            shown_legend = True
        else:
            plt.axvline(rtt, color='red', alpha=0.3, linewidth=1, linestyle='-')

    print(f"RTT > 200 us count: {len(rtt_over_200)} / ({len(rtt_list)})")

    # Dummy bars for legend
    plt.bar(0, 0, color='skyblue', label='RTT ≤ 200 us')
    plt.bar(0, 0, color='tomato', label='RTT > 200 us (bin)')
    plt.plot([], [], ' ', label=f'RTT > 200 us: {len(rtt_over_200)} / {len(rtt_list)}')


    plt.xlim(0, MAX_RTT + BIN_US_WIDTH)
    plt.title(f'RTT Histogram (width = {BIN_US_WIDTH} us, {len(rtt_list)} cycle)')
    plt.xlabel('RTT Time (us)')
    plt.ylabel('Frequency')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    
    # 描画後の plt.show() の代わりに保存する
    plt.savefig(save_filename)
    print(f"Saved histogram plot as {save_filename}")

if __name__ == "__main__":
    main()
