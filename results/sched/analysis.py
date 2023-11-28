import argparse
import pandas as pd
from pathlib import Path


def make_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_path", type=Path)
    return parser


def main():
    parser = make_parser()
    args = parser.parse_args()

    df = pd.read_csv(args.input_path)
    df["delta"] = df["end-start"] * (1_000 / (2 ** 16))
    df["delta_per_count"] = df["delta"] / df["count"]
    df["sched-delta"] = df["delta"] - 0.0010962361462802433 * df["count"]
    print(df.describe())


if __name__ == "__main__":
    main()