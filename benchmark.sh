#!/usr/bin/env bash
RCAL_PY_ITERATIONS=100000
SYSBENCH_THREADS=1000
SYSBENCH_TIME=60 # Seconds
SLEEP_TIME=60 # Seconds

# Array to hold start/end times for time, calibrate, and sysbench
declare -A start_times
declare -A end_times

# Gets the output, start time, end time, and dmesg output of the program in the first parameter
# $1 - The program to run
# $2 - Short name of the program (used as the filenames)
# $3 - The directory to write the output to
function benchmark {
    echo "Running '$1' and writing output to '$2'"

    # Get start time in seconds and YYYY-MM-DD HH:MM:SS
    start_time=$(awk '{print $1}' /proc/uptime)
    start_time_formatted=$(date +"%Y-%m-%d %H:%M:%S")

    # Write the start time in seconds to a file
    echo "Start: $start_time" > "$3/$2.txt"

    # Run the program and write its output to a file
    $1 >> "$3/$2.txt" 3>&1

    # Get the end time in seconds and YYYY-MM-DD HH:MM:SS
    end_time=$(awk '{print $1}' /proc/uptime)
    end_time_formatted=$(date +"%Y-%m-%d %H:%M:%S")

    # Write the end time in seconds to a file
    echo "End: $end_time" >> "$3/$2.txt"
    echo "" >> "$3/$2.txt"

    # Store the start and end times in the arrays
    start_times[$2]=$start_time_formatted
    end_times[$2]=$end_time_formatted

    # Wait one second for dmesg to catch up and then write dmesg output to the file "$2/dmesg_$1"
    sleep 1
    journalctl --boot --grep="\[RCAL\]" --since="$start_time_formatted" --until="$end_time_formatted" --output=short-monotonic > "$3/dmesg_$2.txt"
}

# Make sure we're running as root
if [ "$EUID" -ne 0 ]
  then echo "Script must be run as root to access dmesg, run insmod, etc"
  exit
fi

# Set up the result dir
RESULTS_DIR="results/$(date +%Y-%m-%d-%H-%M-%S)"
mkdir -p "$RESULTS_DIR"

# Unload the module since it may have been updated
if lsmod | grep -q rcal; then
    rmmod rcal
fi

echo -ne "Beginning make"
make all --quiet
echo -ne "...Finished\n"

insmod rcal.ko

benchmark "python rcal.py calibrate $RCAL_PY_ITERATIONS" "calibrate" "$RESULTS_DIR"
benchmark "python rcal.py time $RCAL_PY_ITERATIONS" "time" "$RESULTS_DIR"

# Sleep between benchmarks to let the system settle and have a "clean" dmesg
echo "Sleeping for $SLEEP_TIME seconds..."
sleep $SLEEP_TIME
benchmark "sysbench --threads=$SYSBENCH_THREADS --time=$SYSBENCH_TIME --rand-seed=20 threads run" "sysbench" "$RESULTS_DIR"

# Create dmesg_full.txt which runs from calibrate's start time to sysbench's end time
echo "Creating dmesg_full.txt which runs from calibrate's start time (${start_times[calibrate]}) to sysbench's end time (${end_times[sysbench]})..."
journalctl --boot --grep="\[RCAL\]" --since="${start_times[calibrate]}" --until="${end_times[sysbench]}" --output=short-monotonic > "$RESULTS_DIR/dmesg_full.txt"

# Create dmesg_sleep.txt which runs from time's end time to sysbench's start time
echo "Creating dmesg_sleep.txt which runs from time's end time (${end_times[calibrate]}) to sysbench's start time (${start_times[sysbench]})..."
journalctl --boot --grep="\[RCAL\]" --since="${end_times[time]}" --until="${start_times[sysbench]}" --output=short-monotonic > "$RESULTS_DIR/dmesg_sleep.txt"

# Make the files comma delimited using sed
# Files are in the format [77985.126714] debian kernel: [RCAL] {"start": 1686323097, "end": 1686324186, "count": 15811}
# We want to convert it to 77985.126714,1686323097,1686324186,15811
echo "Comma delimiting files..."
sed -r -i 's/\[([0-9]+\.[0-9]+)\]  debian kernel: \[RCAL\] \{\"start\": ([0-9]+), \"end\": ([0-9]+), \"count\": ([0-9]+)\}/\1,\2,\3,\4/g' "$RESULTS_DIR"/dmesg_*.txt

# Move the dmest_*.txt files to dmesg_*.csv
echo "Renaming files to .csv..."
for file in "$RESULTS_DIR"/dmesg_*.txt; do
    mv -- "$file" "${file%.txt}.csv"
done

# Add headers for time,start,end,count,diff
echo "Adding headers to files..."
sed -i '1s/^/time,start,end,count,diff\n/' "$RESULTS_DIR"/dmesg_*.csv

# Add diff column to all files
echo "Adding diff column to files..."
for file in "$RESULTS_DIR"/dmesg_*.csv; do
    awk -F, '{if (NR==1) print $0; else print $0","$3-$2}' OFS=, "$file" > "$file.tmp"
    mv "$file.tmp" "$file"
done

# Move time* and calibrate* directories to the results directory
echo "Moving time* and calibrate* directories to the results directory..."
mv time* "$RESULTS_DIR"
mv calibrate* "$RESULTS_DIR"
