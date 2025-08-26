import matplotlib.pyplot as plt
import sys
import csv
import os

def parse_ts(ts_str):
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
        print(f"Usage: {sys.argv[0]} user_log.csv kernel_log.csv over_us")
        sys.exit(1)

    user_file = sys.argv[1]
    kernel_file = sys.argv[2]
    over_us = int(sys.argv[3])
    over_ns = over_us * 1_000

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

    # RTT > threshold のデータを抽出
    records = []
    for i, (u, k) in enumerate(zip(user_lines, kernel_lines)):
        u_ss = parse_ts(u[0])
        u_se = parse_ts(u[1])
        u_rs = parse_ts(u[2])
        u_re = parse_ts(u[3])
        k_ss = parse_ts(k[0])
        k_re = parse_ts(k[1])

        rtt = u_re - u_ss
        if rtt > over_ns:
            records.append({
                "index": i+1,
                "u_ss": u_ss, "u_se": u_se, "u_rs": u_rs, "u_re": u_re,
                "k_ss": k_ss, "k_re": k_re,
                "rtt": rtt
            })

    # 下位5件を選択（RTT が小さい順）
    records = sorted(records, key=lambda x: x["rtt"])[:5]

    plt.figure(figsize=(12, 6))

    for plot_idx, rec in enumerate(records):
        y = plot_idx  # サンプル番号
        y_offset = 0.2  # 縦のずらし幅

        # ユーザ相対時間 (µs)
        u_ss_rel = 0
        u_se_rel = (rec["u_se"] - rec["u_ss"]) / 1_000
        u_rs_rel = (rec["u_rs"] - rec["u_ss"]) / 1_000
        u_re_rel = (rec["u_re"] - rec["u_ss"]) / 1_000

        # カーネル相対時間 (µs)
        k_ss_rel = 0
        k_re_rel = (rec["k_re"] - rec["k_ss"]) / 1_000

        # ユーザ send (青)
        plt.plot([u_ss_rel, u_se_rel], [y + y_offset, y + y_offset], 'o', color='blue')
        plt.text(u_se_rel, y + y_offset + 0.05, auto_unit(u_se_rel * 1_000), fontsize=8, color='blue')

        # ユーザ recv (緑)
        plt.plot([u_rs_rel, u_re_rel], [y, y], 'o', color='green')
        plt.text(u_re_rel, y + 0.05, auto_unit((u_re_rel - u_rs_rel) * 1_000), fontsize=8, color='green')

        # カーネル send→recv (赤)
        plt.plot([k_ss_rel, k_re_rel], [y - y_offset, y - y_offset], 'o', color='red')
        plt.text(k_re_rel, y - y_offset + 0.05, auto_unit((k_re_rel - k_ss_rel) * 1_000), fontsize=8, color='red')

        # サンプル番号表示
        plt.text(-50, y, f"line {rec['index']}", fontsize=8, verticalalignment='center')

    plt.xlabel("Time [µs]")
    plt.ylabel("Sample index (bottom-5 RTT)")
    plt.title(f"User vs Kernel timestamps (RTT > {over_us} µs, bottom-5)")
    plt.yticks(range(len(records)))
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(out_png, dpi=150)
    print(f"Saved figure to {out_png}")

if __name__ == "__main__":
    main()
