import matplotlib.pyplot as plt
import sys
import csv
import os

def parse_ts(ts_str):
    """ 'sec.nsec' をナノ秒に変換 """
    sec, nsec = ts_str.split(".")
    return int(sec) * 1_000_000_000 + int(nsec)

def auto_unit(ns):
    if ns > 1_000_000:
        return f"{ns/1_000_000:.2f} ms"
    elif ns > 1_000:
        return f"{ns/1_000:.2f} µs"
    else:
        return f"{ns} ns"

def main():
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} user_log.csv kernel_log.csv output.png")
        sys.exit(1)

    user_file = sys.argv[1]
    kernel_file = sys.argv[2]
    over_us = int(sys.argv[3]) if len(sys.argv) > 0 else 2_000  # デフォルトは2000us
    over_ns = over_us * 1_000  # マイクロ秒をナノ秒に変換

    base_user = os.path.basename(user_file)
    base_kernel = os.path.basename(kernel_file)

    common_prefix = os.path.commonprefix([base_user, base_kernel])
    out_png = f"{common_prefix}over{over_us}.png"


    # CSV読み込み
    with open(user_file) as f:
        user_lines = [row for row in csv.reader(f) if row]
    with open(kernel_file) as f:
        kernel_lines = [row for row in csv.reader(f) if row]

    assert len(user_lines) == len(kernel_lines), "ユーザログとカーネルログの行数が一致しません"

    # RTTを計算してリスト化
    records = []
    for i, (u, k) in enumerate(zip(user_lines, kernel_lines)):
        u_ss = parse_ts(u[0])
        u_se = parse_ts(u[1])
        u_rs = parse_ts(u[2])
        u_re = parse_ts(u[3])
        k_ss = parse_ts(k[0])
        k_re = parse_ts(k[1])

        rtt_ns = u_re - u_ss
        if rtt_ns > over_ns:  # 1000 µs
            records.append({
                "index": i+1,  # 元の行番号
                "u_ss": u_ss, "u_se": u_se, "u_rs": u_rs, "u_re": u_re,
                "k_ss": k_ss, "k_re": k_re,
                "rtt": rtt_ns
            })

    # RTTの大きい順にソートして下5件
    records = sorted(records, key=lambda x: x["rtt"])[:5]
    plt.yticks(range(len(records)))

    # プロット
    plt.figure(figsize=(12, 6))
    for plot_idx, rec in enumerate(records):
        base = rec["u_ss"]
        u_ss = rec["u_ss"] - base
        u_se = rec["u_se"] - base
        u_rs = rec["u_rs"] - base
        u_re = rec["u_re"] - base
        k_ss = rec["k_ss"] - base
        k_re = rec["k_re"] - base

        y = plot_idx
        offset = 0.2

        # user send
        plt.hlines(y + offset, u_ss/1e6, u_se/1e6, colors='blue', linewidth=4,
                   label="SOEM send" if plot_idx==0 else "")
        plt.text(u_se/1e6, y + offset + 0.05, auto_unit(u_se - u_ss), fontsize=8, color='blue')

        # kernel send→recv
        plt.hlines(y, k_ss/1e6, k_re/1e6, colors='red', linewidth=4,
                   label="kernel send→recv" if plot_idx==0 else "")
        plt.text(k_re/1e6, y + 0.05, auto_unit(k_re - k_ss), fontsize=8, color='red')

        # user recv
        plt.hlines(y-offset, u_rs/1e6, u_re/1e6, colors='green', linewidth=4,
                   label="SOEM recv" if plot_idx==0 else "")
        plt.text(u_re/1e6, y - offset + 0.05, auto_unit(u_re - u_rs), fontsize=8, color='green')

        # 行番号とRTT表示
        plt.text(u_ss/1e6, y - 0.4,
                 f"line {rec['index']} (RTT={auto_unit(rec['rtt'])})",
                 fontsize=8, color='black')

    plt.xlabel("Time [ms] (relative to user send_start)")
    plt.ylabel(f"5 sample (RTT > {over_us} us)")
    plt.title(f"Round-trip transmission and reception processing time (RTT > {over_us} us)")
    plt.legend()
    plt.tight_layout()

    # PNG保存
    plt.savefig(out_png, dpi=150)
    print(f"Saved figure to {out_png}")

if __name__ == "__main__":
    main()
