import json
import argparse
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from datetime import datetime
from tqdm import tqdm


def lscpu():
    result = subprocess.run(["lscpu"], capture_output=True, text=True)
    return result.stdout.strip()


def uname():
    result = subprocess.run(["uname", "-a"], capture_output=True, text=True)
    return result.stdout.strip()


def sync(args):
    timestamp = datetime.now()
    out_path = Path.cwd().joinpath(f"sync-{timestamp}")
    if out_path.exists():
        print("Error: output path already exists")
        exit(1)
    out_path.mkdir(parents=True, exist_ok=True)

    source = Path("/sys/kernel/rcal/rcal_calibrate")
    if not source.exists():
        print("Error: rcal module not loaded")
        exit(1)
    
    try:
        sample = json.loads(source.read_text())
    except json.JSONDecodeError:
        print("Error: rcal emitted bad JSON")
        exit(1)
    
    if "error" in sample:
        error = sample["error"]
        print(f"Error: rcal error: {error}")
        exit(1)
    
    print("Data collection:")
    data = []
    for _ in tqdm(range(args.samples)):
        sample = json.loads(source.read_text())
        delta = (sample["end"] - sample["start"] if sample["end"] >= sample["start"] else ((1 << 32) - 1) - sample["end"] + sample["start"]) * (1_000 / (2 ** sample["units"]))
        datum = {
            "delta": delta,
            "count": sample["count"],
            "delta_per_count": delta / sample["count"],
        }
        data.append(datum)

    out_df = out_path.joinpath("data.csv")
    df = pd.DataFrame(data)
    df.to_csv(out_df, index=False)

    out_platform = out_path.joinpath("lscpu.txt")
    out_platform.write_text(lscpu())

    out_platform = out_path.joinpath("uname.txt")
    out_platform.write_text(uname())

    out_summary = out_path.joinpath("summary.csv")
    summary = df.describe()
    print("Summary statistics:")
    print(summary)
    summary.to_csv(out_summary, header=True)

    out_plot = out_path.joinpath("plot.png")
    _, axes = plt.subplots(nrows=2, ncols=2, figsize=(20, 12))
    df[["delta"]].hist(ax=axes[0, 0], bins=20)
    axes[0, 0].set_title("Energy Consumption")
    axes[0, 0].set_xlabel("Delta (nano-Joules)")
    axes[0, 0].set_ylabel("Bin Count")
    df[["count"]].hist(ax=axes[0, 1], bins=20)
    axes[0, 1].set_title("Increment Operations")
    axes[0, 1].set_xlabel("Count")
    axes[0, 1].set_ylabel("Bin Count")
    df[["delta_per_count"]].hist(ax=axes[1, 0], bins=20)
    axes[1, 0].set_title("Energy Consumption per Increment Operation")
    axes[1, 0].set_xlabel("Delta per Count (nano-Joules per increment)")
    axes[1, 0].set_ylabel("Bin Count")
    df.plot.scatter(ax=axes[1, 1], x="count", y="delta")
    axes[1, 1].set_title("Energy Consumption per Increment Operation")
    axes[1, 1].set_xlabel("Count")
    axes[1, 1].set_ylabel("Delta (nano-Joules)")
    plt.savefig(out_plot)
    plt.show()


def make_parser():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(title="commands")

    sync_parser = subparsers.add_parser("sync")
    sync_parser.add_argument("samples", type=int)
    sync_parser.set_defaults(func=sync)

    return parser


def main():
    parser = make_parser()
    args = parser.parse_args()

    if "func" not in args:
        parser.print_help()
        exit(1)
    
    args.func(args)


if __name__ == "__main__":
    main()