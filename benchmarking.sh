#!/usr/bin/env bash
RCAL_PY_ITERATIONS=100000
SYSBENCH_THREADS=1000
SYSBENCH_TIME=60 # Seconds
SLEEP_TIME=60 # Seconds

# Gets the output, start time, end time, and dmesg output of the program in the first parameter
function benchmark {
    echo "Running '$1'"

    # Write the start time in seconds to a file
    awk '{print "Start:", $1}' /proc/uptime > $2

    # Run the program in the first parameter and write the output to the file in the second parameter
    $1 >> $2 3>&1

    # Write the end time in seconds to a file
    awk '{print "End:", $1}' /proc/uptime >> $2
    echo "" >> $2

    # Wait one second for dmesg to catch up and then write dmesg output to the file
    sleep 1
    dmesg -l info | grep '[RCAL]' >> $2

}

# Make sure we're running as root
if [ "$EUID" -ne 0 ]
  then echo "Script must be run as root to access dmesg, run insmod, etc"
  exit
fi

# Set up the result dir
RESULTS_DIR="results-$(date +%Y-%m-%d-%H-%M-%S)"
mkdir $RESULTS_DIR

# Unload the module since it may have been updated
if lsmod | grep -q rcal; then
    rmmod rcal
fi

echo -ne "Beginning make"
make all --quiet
# Echo "...Finished" on the same line
echo -ne "...Finished\n"

insmod rcal.ko

benchmark "python rcal.py calibrate $RCAL_PY_ITERATIONS" "$RESULTS_DIR/calibrate.txt"
benchmark "python rcal.py time $RCAL_PY_ITERATIONS" "$RESULTS_DIR/time.txt"

# Sleep between benchmarks to let the system settle and have a "clean" dmesg
echo "Sleeping for $SLEEP_TIME seconds..."
sleep $SLEEP_TIME
benchmark "sysbench --threads=$SYSBENCH_THREADS --time=$SYSBENCH_TIME --rand-seed=20 threads run" "$RESULTS_DIR/sysbench.txt"
