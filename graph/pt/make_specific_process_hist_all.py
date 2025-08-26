#!/usr/bin/env python3
import sys
import os
import matplotlib.pyplot as plt
import numpy as np

BIN_WIDTH_US = 10

def read_kernel_file(filename):
    return np.loadtxt(filename)

def read_user_file(filename):
    data = np.loadtxt(filename, delimiter=",")
    # data[:,0]=send, data[:,1]=poll_recv, data[:,2]=recv, data[:,3]=total
    return data

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} user_log kernel_log")
        sys.exit(1)

    user_file, kernel_file = sys.argv[1], sys.argv[2]

    # 共通部分から出力ファイル名生成
    basename1 = os.path.splitext(os.path.basename(user_file))[0]
    basename2 = os.path.splitext(os.path.basename(kernel_file))[0]
    common = os.path.commonprefix([basename1, basename2])
    out_file = common + "all_hist.png"

    # データ読み込み
    user_data = read_user_file(user_file)   # shape=(N,4)
    kernel_data = read_kernel_file(kernel_file)

    if len(user_data) != len(kernel_data):
        print("Error: user と kernel のサンプル数が一致しません")
        sys.exit(1)

    # 系列とラベルをまとめる
    datasets = [
        (user_data[:,0], "SOEM Send"),
        (user_data[:,1], "SOEM Poll"),
        (user_data[:,2], "SOEM Recv"),
        (user_data[:,3], "SOEM Total"),
        (kernel_data,   "Kernel")
    ]

    max_rtt = np.max(user_data[:,3])

    # 色リスト（系列ごとに違う色を割当て）
    colors = ["skyblue", "orange", "lightgreen", "violet", "salmon"]

    # サブプロット作成（縦に5つ並べる）
    fig, axes = plt.subplots(len(datasets), 1, figsize=(10, 12), sharex=False)

    for ax, (sample, label), color in zip(axes, datasets, colors):
        min_val, max_val = np.min(sample), np.max(sample)
        bins = np.arange(min_val, max_val + BIN_WIDTH_US, BIN_WIDTH_US) 

        ax.hist(sample, bins=bins, color=color, alpha=0.7, label=label)
        ax.set_xlim(0, max_rtt + 10)
        ax.set_ylabel("Frequency (log scale)")
        ax.set_yscale('log')
        ax.set_title(f"{label} ({len(sample)} samples)")
        ax.legend()

    axes[-1].set_xlabel("Processing Time [us]")

    plt.title(f"Histgram of Processes in EtherCAT Round trip (bin={BIN_WIDTH_US}us, {len(user_data)} samples)")
    plt.tight_layout()
    plt.savefig(out_file)
    print(f"Saved: {out_file}")

if __name__ == "__main__":
    main()
