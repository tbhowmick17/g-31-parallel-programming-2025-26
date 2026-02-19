#!/usr/bin/env bash
#SBATCH --job-name=matmul

####### Node Info #######
#SBATCH --exclusive
#SBATCH --nodes=8
#SBATCH --ntasks-per-node=64

####### Ressources #######
#SBATCH --time=00:30:00

####### Partition #######
#SBATCH --partition=all

####### Output #######
#SBATCH --output=/home/fd0003356/out/matmul_speedup.out.%j
#SBATCH --error=/home/fd0003356/out/matmul_speedup.err.%j

# Load modules 
module load gcc/14.3.0
module load openmpi

# Parameters
N=8000
SEED=42
VERBOSE=0

# Calculate total processes
NPROCS=$((SLURM_JOB_NUM_NODES * 64))

echo "=========================================="
echo "MPI Matrix Multiplication"
echo "=========================================="
echo "Nodes: $SLURM_JOB_NUM_NODES"
echo "Processes per node: 64"
echo "Total processes: $NPROCS"
echo "Matrix size: ${N}x${N}"
echo "Seed: $SEED"
echo "=========================================="
echo ""

# Run the program
mpirun -np $NPROCS ./matmul $N $SEED $VERBOSE

echo ""
echo "Job completed at: $(date)"
