#!/usr/bin/env python3
import sys
import re
import os

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} input_file.log")
        sys.exit(1)

    input_file = sys.argv[1]

    # 出力ファイル名を決定
    if input_file.endswith(".log"):
        output_file = input_file[:-4] + "_diff.log"
    else:
        output_file = input_file + "_diff.log"

    # sys_enter_ppoll と sys_exit_ppoll のログ行を解析するための正規表現
    pattern = re.compile(r'^\s*\S+\s+\[\d+\]\s+(\d+\.\d+):\s+(sys_enter_ppoll|sys_exit_ppoll)')

    enter_ts = None

    with open(input_file, "r") as fin, open(output_file, "w") as fout:
        for line in fin:
            m = pattern.match(line)
            if not m:
                continue

            ts = float(m.group(1))
            event = m.group(2)

            if event == "sys_enter_ppoll":
                enter_ts = ts
            elif event == "sys_exit_ppoll" and enter_ts is not None:
                diff_us = (ts - enter_ts) * 1_000_000  # 秒差分をマイクロ秒に変換
                fout.write(f"{enter_ts:.6f},{ts:.6f},{diff_us:.3f}\n")
                enter_ts = None  # 次のペアに備える

    print(f"差分を {output_file} に出力しました")

if __name__ == "__main__":
    main()
