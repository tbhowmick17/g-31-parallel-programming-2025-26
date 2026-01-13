#!/bin/bash
#SBATCH --job-name=heatmap_speedup
#SBATCH --output=heatmap_speedup.out
#SBATCH --error=heatmap_speedup.err
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64
#SBATCH --time=00:20:00
#SBATCH --partition=compute

# ------------------------------------------------------------
# Load compiler
# ------------------------------------------------------------
module load gcc

# ------------------------------------------------------------
# Build program
# ------------------------------------------------------------
gcc -O3 -fopenmp heatmap_analysis.c -o heatmap_analysis

# ------------------------------------------------------------
# Problem parameters (FIXED for speedup measurement)
# ------------------------------------------------------------
COLUMNS=1024
ROWS=786
SEED=1337
LOWER=0
UPPER=100
WINDOW_HEIGHT=20
VERBOSE=0
WORK_FACTOR=10

# ------------------------------------------------------------
# Measurement setup
# ------------------------------------------------------------
THREADS_LIST="1 2 4 8 16 32 64"
ITERATIONS=5

echo "threads,iteration,time_seconds"

# ------------------------------------------------------------
# Run measurements
# ------------------------------------------------------------
for T in $THREADS_LIST; do
    export OMP_NUM_THREADS=$T
    export OMP_PROC_BIND=close
    export OMP_PLACES=cores

    for ((i=1; i<=ITERATIONS; i++)); do
        ./heatmap_analysis \
            $COLUMNS $ROWS $SEED $LOWER $UPPER \
            $WINDOW_HEIGHT $VERBOSE $T $WORK_FACTOR
    done
done
