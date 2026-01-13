#!/usr/bin/env bash
####### Mail Notify / Job Name / Comment #######
#SBATCH --job-name="heatmap_omp"

####### Partition #######
#SBATCH --partition=all

####### Ressources #######
#SBATCH --time=0-00:20:00

####### Node Info #######
#SBATCH --exclusive
#SBATCH --nodes=1

####### Output #######
#SBATCH --output=/home/fd0005400/out/heatmap_speedup.out.%j
#SBATCH --error=/home/fd0005400/out/heatmap_speedup.err.%j

# ------------------------------------------------------------
# Load compiler
# ------------------------------------------------------------
module load gcc

# ------------------------------------------------------------
# Compile
# ------------------------------------------------------------
gcc -O3 -fopenmp heatmap_analysis.c -o heatmap_analysis

# ------------------------------------------------------------
# Fixed parameters (for speedup measurement)
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

export OMP_PROC_BIND=close
export OMP_PLACES=cores

echo "threads,iteration,time_seconds"

# ------------------------------------------------------------
# Run experiments
# ------------------------------------------------------------
for T in $THREADS_LIST; do
    export OMP_NUM_THREADS=$T

    for ((i=1; i<=ITERATIONS; i++)); do
        ./heatmap_analysis \
            $COLUMNS $ROWS $SEED $LOWER $UPPER \
            $WINDOW_HEIGHT $VERBOSE $T $WORK_FACTOR
    done
done
