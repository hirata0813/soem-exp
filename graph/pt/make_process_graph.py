#!/usr/bin/env python3
import sys
import os
import matplotlib.pyplot as plt
import numpy as np

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
    out_file = common + "_hist.png"

    # データ読み込み
    user_data = read_user_file(user_file)   # shape=(N,4)
    kernel_data = read_kernel_file(kernel_file)

    if user_data.shape[0] != kernel_data.shape[0]:
        print("Error: user と kernel のサンプル数が一致しません")
        sys.exit(1)

    # 各成分
    sends = user_data[:,0]
    recvs = user_data[:,2]
    kernels = kernel_data
    totals = user_data[:,3]

    # ヒストグラム描画
    fig, ax = plt.subplots(figsize=(10,6))

    bins = 100  # ビン数は調整可能
    ax.hist([sends, kernels, recvs],
            bins=bins,
            stacked=True,
            label=["User Send", "Kernel", "User Recv"],
            color=["skyblue","lightgreen","lightcoral"])

    ax.set_xlabel("処理時間 [us] (Total)")
    ax.set_ylabel("Frequency")
    ax.set_title(f"処理時間分布 (積み上げ内訳付き): {len(totals)} samples")
    ax.legend()

    plt.tight_layout()
    plt.savefig(out_file)
    print(f"Saved: {out_file}")

if __name__ == "__main__":
    main()
