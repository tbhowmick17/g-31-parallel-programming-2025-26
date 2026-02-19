#!/usr/bin/env bash
# Simple batch script to submit all required experiments

N=8000
SEED=42
VERBOSE=0

# Node configurations (as per assignment)
declare -a NODES=(1 2 4 6 8)

echo "Submitting jobs for all configurations..."
echo ""

for nodes in "${NODES[@]}"; do
    echo "Submitting: ${nodes} nodes, 64 processes/node (total: $((nodes * 64)))"
    
    sbatch <<EOF
#!/usr/bin/env bash
#SBATCH --job-name=matmul_n${nodes}
#SBATCH --output=/home/fd0003356/out/matmul_n${nodes}_%j.out
#SBATCH --error=/home/fd0003356/out/matmul_n${nodes}_%j.err
#SBATCH --nodes=${nodes}
#SBATCH --exclusive
#SBATCH --ntasks-per-node=64
#SBATCH --time=00:30:00
#SBATCH --partition=all

module load gcc/14.3.0
module load openmpi

NPROCS=\$((${nodes} * 64))
echo "Running with ${nodes} nodes, \$NPROCS processes"
mpirun -np \$NPROCS ./matmul ${N} ${SEED} ${VERBOSE}
EOF

    sleep 1
done

echo ""
echo "All jobs submitted!"
echo "Check status with: squeue -u \$USER"
echo "View output with: ls -lh matmul_*.out"