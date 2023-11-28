#!/usr/bin/env bash
RCAL_PY_ITERATIONS=100000
SYSBENCH_THREADS=1000
SYSBENCH_TIME=60 # Seconds
SLEEP_TIME=60 # Seconds

# Gets the output, start time, end time, and dmesg output of the program in the first parameter
# $1 - The program to run
# $2 - Short name of the program (used as the filenames)
# $3 - The directory to write the output to
function benchmark {
    echo "Running '$1' and writing output to '$2'"

    # Write the start time in seconds to a file
    awk '{print "Start:", $1}' /proc/uptime > "$3/$2.txt"

    # Run the program and write its output to a file
    $1 >> "$3/$2.txt" 3>&1

    # Write the end time in seconds to a file
    awk '{print "End:", $1}' /proc/uptime >> "$3/$2.txt"
    echo "" >> "$3/$2.txt"

    # Wait one second for dmesg to catch up and then write dmesg output to the file "$2/dmesg_$1"
    sleep 1
    dmesg -l info | grep '[RCAL]' >> "$3/dmesg_$2.txt"
}

# Make sure we're running as root
if [ "$EUID" -ne 0 ]
  then echo "Script must be run as root to access dmesg, run insmod, etc"
  exit
fi

# Set up the result dir
RESULTS_DIR="results/$(date +%Y-%m-%d-%H-%M-%S)"
mkdir -p $RESULTS_DIR

# Unload the module since it may have been updated
if lsmod | grep -q rcal; then
    rmmod rcal
fi

echo -ne "Beginning make"
make all --quiet
# Echo "...Finished" on the same line
echo -ne "...Finished\n"

insmod rcal.ko

benchmark "python rcal.py calibrate $RCAL_PY_ITERATIONS" "calibrate" "$RESULTS_DIR"
benchmark "python rcal.py time $RCAL_PY_ITERATIONS" "time" "$RESULTS_DIR"

# Sleep between benchmarks to let the system settle and have a "clean" dmesg
echo "Sleeping for $SLEEP_TIME seconds..."
sleep $SLEEP_TIME
benchmark "sysbench --threads=$SYSBENCH_THREADS --time=$SYSBENCH_TIME --rand-seed=20 threads run" "sysbench" "$RESULTS_DIR"

# Consolidate dmesg output
cat $RESULTS_DIR/dmesg_*.txt > $RESULTS_DIR/dmesg.txt
rm $RESULTS_DIR/dmesg_*.txt

# Sort the dmesg output by time and remove duplicates
sort -n $RESULTS_DIR/dmesg.txt | uniq > $RESULTS_DIR/dmesg_sorted.txt

# Make the file comma delimited using sed
# File is in the format [77985.126714] [RCAL] {"start": 1686323097, "end": 1686324186, "count": 15811}
# We want to convert it to 77985.126714,1686323097,1686324186,15811
sed -r 's/\[([0-9]+\.[0-9]+)\] \[RCAL\] \{\"start\": ([0-9]+), \"end\": ([0-9]+), \"count\": ([0-9]+)\}/\1,\2,\3,\4/g' $RESULTS_DIR/dmesg_sorted.txt > $RESULTS_DIR/dmesg.csv

# Move time* and calibrate* directories to the results directory
mv time* $RESULTS_DIR
mv calibrate* $RESULTS_DIR
