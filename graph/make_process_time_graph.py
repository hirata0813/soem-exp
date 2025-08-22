#!/usr/bin/env python3
import sys
import matplotlib.pyplot as plt

def parse_time(tstr):
    """sec.nanosec を float 秒に変換"""
    sec, nsec = tstr.split(".")
    return int(sec) + int(nsec) * 1e-9

def read_user_log(path):
    send_times, recv_times, total_times = [], [], []
    with open(path) as f:
        for line in f:
            parts = line.strip().split(",")
            if len(parts) != 4:
                continue
            send_start = parse_time(parts[0])
            send_end   = parse_time(parts[1])
            recv_start = parse_time(parts[2])
            recv_end   = parse_time(parts[3])

            send_times.append(send_end - send_start)
            recv_times.append(recv_end - recv_start)
            total_times.append(recv_end - send_start)
    return send_times, recv_times, total_times

def read_kernel_log(path):
    total_times = []
    with open(path) as f:
        for line in f:
            parts = line.strip().split(",")
            if len(parts) != 2:
                continue
            send_start = parse_time(parts[0])
            recv_end   = parse_time(parts[1])
            total_times.append(recv_end - send_start)
    return total_times

def average(lst):
    return sum(lst) / len(lst) if lst else 0.0

def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} user_log kernel_log output.png")
        sys.exit(1)

    user_log = sys.argv[1]
    kernel_log = sys.argv[2]
    out_file = sys.argv[3]

    send_times, recv_times, user_total = read_user_log(user_log)
    kernel_total = read_kernel_log(kernel_log)

    averages = [
        average(send_times),
        average(recv_times),
        average(user_total),
        average(kernel_total),
    ]
    labels = [
        "User Send",
        "User Recv",
        "User Total",
        "Kernel Total",
    ]

    plt.figure(figsize=(8, 6))
    plt.bar(labels, [v * 1e6 for v in averages])  # μsに変換
    plt.ylabel("Time (µs)")
    plt.title("Average Processing Times")
    plt.grid(axis="y", linestyle="--", alpha=0.7)

    for i, v in enumerate(averages):
        plt.text(i, v*1e6, f"{v*1e6:.1f}", ha="center", va="bottom")

    plt.tight_layout()
    plt.savefig(out_file)

if __name__ == "__main__":
    main()
