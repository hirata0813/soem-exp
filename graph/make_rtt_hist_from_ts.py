import sys
import matplotlib.pyplot as plt
import numpy as np
import os

MAX_RTT = 500  # us
BIN_US_WIDTH = 5  # Width of histogram bins in microseconds

def parse_timestamp(ts_str):
    """
    "sec.nsec" 形式の文字列を受け取り、マイクロ秒に変換した整数を返す
    """
    if '.' in ts_str:
        sec_str, nsec_str = ts_str.split('.', 1)
        sec = int(sec_str)
        nsec = int(nsec_str.ljust(9, '0')[:9])  # 常にナノ秒9桁に調整
    else:
        sec = int(ts_str)
        nsec = 0
    total_us = sec * 1_000_000 + nsec // 1000  # μs に変換
    return total_us

def load_rtt_from_soem(filepath):
    """ soem ファイル用: ts1, ts4 の差 """
    rtt_list = []
    with open(filepath, 'r') as f:
        for line in f:
            if not line.strip():
                continue
            parts = [p.strip() for p in line.split(',')]
            if len(parts) < 4:
                continue
            try:
                ts1_us = parse_timestamp(parts[0])
                ts4_us = parse_timestamp(parts[3])
                rtt = ts4_us - ts1_us
                rtt_list.append(rtt)
            except ValueError:
                continue
    return rtt_list

def load_rtt_from_ws(filepath):
    """ ws ファイル用: ts1, ts2 の差 """
    rtt_list = []
    with open(filepath, 'r') as f:
        for line in f:
            if not line.strip():
                continue
            parts = [p.strip() for p in line.split(',')]
            if len(parts) < 2:
                continue
            try:
                ts1_us = parse_timestamp(parts[0])
                ts2_us = parse_timestamp(parts[1])
                rtt = ts2_us - ts1_us
                rtt_list.append(rtt)
            except ValueError:
                continue
    return rtt_list

def plot_rtt_histogram(rtt_list, save_filename):
    mean_rtt = np.mean(rtt_list)
    max_rtt = max(rtt_list)
    min_rtt = min(rtt_list)
    print(f"Mean RTT: {mean_rtt:.2f} us\nMax RTT: {max_rtt:.2f} us\nMin RTT: {min_rtt:.2f} us")

    bins = np.arange(0, MAX_RTT + BIN_US_WIDTH, BIN_US_WIDTH)
    counts, edges = np.histogram(rtt_list, bins=bins)
    bar_colors = ['skyblue' if edge <= 200 else 'tomato' for edge in edges[:-1]]

    plt.figure(figsize=(10, 6))
    plt.bar(edges[:-1], counts, width=np.diff(edges), align='edge',
            color=bar_colors, edgecolor='black')

    plt.axvline(mean_rtt, color='green', linestyle='dashed', linewidth=2,
                label=f'Mean RTT: {mean_rtt:.2f} us')
    plt.plot([], [], ' ', label=f'Min RTT: {min_rtt:.2f} us')
    plt.plot([], [], ' ', label=f'Max RTT: {max_rtt:.2f} us')

    rtt_over_1000 = [rtt for rtt in rtt_list if rtt > 1000]
    rtt_over_200 = [rtt for rtt in rtt_list if rtt > 200]
    shown_legend = False
    for rtt in rtt_over_200:
        if not shown_legend:
            plt.axvline(rtt, color='red', alpha=0.3, linewidth=1,
                        linestyle='-', label='RTT > 200 us')
            shown_legend = True
        else:
            plt.axvline(rtt, color='red', alpha=0.3, linewidth=1, linestyle='-')

    print("[INFO] RTT > 1000 us")
    for rtt in rtt_over_1000:
        print(f"  {rtt} us") 
    print(f"RTT > 200 us count: {len(rtt_over_200)} / ({len(rtt_list)})")
    print(f"RTT > 1000 us count: {len(rtt_over_1000)} / {len(rtt_list)}")

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
    plt.savefig(save_filename)
    print(f"Saved histogram plot as {save_filename}")

def main():
    if len(sys.argv) != 2:
        print(f"Usage: python {sys.argv[0]} <timestamp data file>")
        sys.exit(1)

    filepath = sys.argv[1]
    filename = os.path.basename(filepath)

    if "soem" in filename:
        rtt_list = load_rtt_from_soem(filepath)
    elif "ws" in filename:
        rtt_list = load_rtt_from_ws(filepath)
    else:
        print("Error: filename must contain 'soem' or 'ws'")
        sys.exit(1)

    if not rtt_list:
        print("No RTT data found.")
        sys.exit(1)

    base = os.path.splitext(filename)[0]
    save_filename = base + '.png'
    plot_rtt_histogram(rtt_list, save_filename)

if __name__ == "__main__":
    main()
