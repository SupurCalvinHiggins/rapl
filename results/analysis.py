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

    dmesg_path = args.input_path.joinpath("sched/dmesg.csv")
    calibrate_path = args.input_path.joinpath("calibrate/summary.csv")

    df = pd.read_csv(dmesg_path)
    cdf = pd.read_csv(calibrate_path)

    df["delta"] = (df["end"] - df["start"]) * (1_000 / (2 ** 16))
    df["delta_per_count"] = df["delta"] / df["count"]
    df["sched_delta"] = df["delta"] - cdf["delta_per_count"][1] * df["count"]
    df = df.drop(["end", "start"], axis=1)
    df = df.describe()

    print(df)
    df.to_csv(args.input_path.joinpath("sched/summary.csv"))


if __name__ == "__main__":
    main()
