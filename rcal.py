import json
import argparse
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from datetime import datetime
from tqdm import tqdm

# TODO: Record kernel information like lscpu
# TODO: Automatically generate output name with timestamp and type
# TODO: print output name


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
    
    data = []
    for _ in tqdm(range(args.samples)):
        sample = json.loads(source.read_text())
        datum = {
            "delta": (sample["end"] - sample["start"] if sample["end"] >= sample["start"] else ((1 << 32) - 1) - sample["end"] + sample["start"]) * (1_000 / (2 ** sample["units"])),
            "count": sample["count"],
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
    print(summary)
    summary.to_csv(out_summary, header=True)

    out_hist = out_path.joinpath("hist.png")
    fig, axes = plt.subplots(nrows=1, ncols=2, figsize=(10, 6))
    df[["delta"]].hist(ax=axes[0], bins=20)
    axes[0].set_title("Energy Consumption")
    axes[0].set_xlabel("Delta (nano-Joules)")
    axes[0].set_ylabel("Bin Count")
    df[["count"]].hist(ax=axes[1], bins=20)
    axes[1].set_title("Increment Operations")
    axes[1].set_xlabel("Count")
    axes[1].set_ylabel("Bin Count")
    plt.savefig(out_hist)
    plt.show()

    out_scatter = out_path.joinpath("scatter.png")
    df.plot.scatter(x="count", y="delta")
    plt.xlabel("Count")
    plt.ylabel("Delta (nano-Joules)")
    plt.title("Energy Consumption per Increment Operation")
    plt.savefig(out_scatter)
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
    print("OK")


if __name__ == "__main__":
    main()