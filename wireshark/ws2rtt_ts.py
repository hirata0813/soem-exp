import sys
import os
import csv

def parse_time(timestamp_str):
    sec, nsec = timestamp_str.strip().split('.')
    total_nsec = int(sec) * 1_000_000_000 + int(nsec[:9].ljust(9, '0'))
    return total_nsec

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 calc_diff_us.py <input_file>")
        sys.exit(1)

    input_path = sys.argv[1]
    base_name = os.path.splitext(os.path.basename(input_path))[0]
    output_file = base_name + ".log"

    with open(input_path, "r") as infile, open(output_file, "w", newline="") as outfile:
        writer = csv.writer(outfile)
        lines = infile.readlines()

        if len(lines) % 2 != 0:
            print("Error: Input file must contain an even number of lines (Tx/Rx pairs).")
            sys.exit(1)

        for i in range(0, len(lines), 2):
            tx_time_ns = float(lines[i])
            rx_time_ns = float(lines[i + 1])
            # diff_us = (rx_time_ns - tx_time_ns) / 1000  # ナノ秒 → マイクロ秒
            writer.writerow(["{:.9f}".format(tx_time_ns), "{:.9f}".format(rx_time_ns)])

if __name__ == "__main__":
    main()
