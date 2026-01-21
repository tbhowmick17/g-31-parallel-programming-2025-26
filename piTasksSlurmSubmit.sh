#!/usr/bin/env bash
####### Mail Notify / Job Name / Comment #######
#SBATCH --job-name="pi_tasks"

####### Partition #######
#SBATCH --partition=all

####### Ressources #######
#SBATCH --time=0-00:20:00

####### Node Info #######
#SBATCH --exclusive
#SBATCH --nodes=1

####### Output #######
#SBATCH --output=/home/fd0003356/out/pi_tasks_speedup.out.%j
#SBATCH --error=/home/fd0003356/out/pi_tasks_speedup.err.%j

module load gcc

gcc -O3 -fopenmp pi_tasks.c -o pi_tasks -lm

NUM_TASKS=20000
LOWER=10000
UPPER=1000000
SEED=42
THREADS_LIST="1 2 4 8 16 32 64"

export OMP_PROC_BIND=close
export OMP_PLACES=cores

echo "Starting speedup measurements..."
echo ""

for T in $THREADS_LIST; do
    export OMP_NUM_THREADS=$T
    
    echo "========================================="
    echo "Running with $T threads"
    echo "========================================="
    ./pi_tasks $NUM_TASKS $T $LOWER $UPPER $SEED
    echo ""
done

echo "Speedup measurements complete!"