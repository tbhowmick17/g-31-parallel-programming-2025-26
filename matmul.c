#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

// ============================================================================
// OPTIMIZED MPI MATRIX MULTIPLICATION
// Key improvements over original code:
// 1. NO master bottleneck - uses collective operations (Scatter/Gather)
// 2. Cache-optimized - transposes B matrix for row-major access
// 3. Static distribution - no dynamic scheduling overhead
// 4. Efficient communication - single broadcast + scatter + gather
// ============================================================================

// Random number generator (same as assignment)
double my_rand(unsigned long *state, double lower, double upper)
{
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    unsigned long x = (*state * 0x2545F4914F6CDD1DULL);
    const double inv = 1.0 / (double)(1ULL << 53);
    double u = (double)(x >> 11) * inv;
    return lower + (upper - lower) * u;
}

unsigned concatenate(unsigned x, unsigned y)
{
    unsigned pow = 10;
    while (y >= pow)
        pow *= 10;
    return x * pow + y;
}

// Fill matrix with random values (assignment specification)
void fillArray(double **arr, int n, int seed_value)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            unsigned long state = concatenate(i, j) + seed_value;
            arr[i][j] = my_rand(&state, 0, 1);
        }
    }
}

// Allocate 2D matrix with contiguous memory
double **allocate_matrix(int rows, int cols)
{
    double **matrix = (double **)malloc(rows * sizeof(double *));
    double *data = (double *)malloc(rows * cols * sizeof(double));
    for (int i = 0; i < rows; i++)
    {
        matrix[i] = &data[i * cols];
    }
    return matrix;
}

// Free matrix
void free_matrix(double **matrix)
{
    if (matrix)
    {
        if (matrix[0])
            free(matrix[0]);
        free(matrix);
    }
}

// Print matrix (verbose mode)
void print_matrix(const char *name, double **matrix, int n)
{
    printf("Matrix %s:\n", name);
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            printf("%.6f ", matrix[i][j]);
        }
        printf("\n");
    }
}

// Compute checksum
double compute_checksum(double **matrix, int n)
{
    double checksum = 0.0;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            checksum += matrix[i][j];
        }
    }
    return checksum;
}

// Matrix multiplication: C = A × B_transpose
// CRITICAL: B is transposed for CACHE EFFICIENCY!
// Original code accesses B column-wise (slow), we access row-wise (fast)
// void multiply_matrices(double **A, double **B_transpose, double **C, int rows, int n)
// {
//     for (int i = 0; i < rows; i++)
//     {
//         for (int j = 0; j < n; j++)
//         {
//             double sum = 0.0;
//             // Both A[i] and B_transpose[j] accessed sequentially: CACHE FRIENDLY!
//             for (int k = 0; k < n; k++)
//             {
//                 sum += A[i][k] * B_transpose[j][k];
//             }
//             C[i][j] = sum;
//         }
//     }
// }

void multiply_matrices(double **A, double **B_transpose, double **C,
                       int rows, int n)
{
#define BLOCK_SIZE 96 // Tune for your CPU's cache

    // Initialize C to zero
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < n; j++)
            C[i][j] = 0.0;

    // Block over all dimensions
    for (int ii = 0; ii < rows; ii += BLOCK_SIZE)
    {
        for (int jj = 0; jj < n; jj += BLOCK_SIZE)
        {
            for (int kk = 0; kk < n; kk += BLOCK_SIZE)
            {
                // Compute block
                int i_max = (ii + BLOCK_SIZE < rows) ? ii + BLOCK_SIZE : rows;
                int j_max = (jj + BLOCK_SIZE < n) ? jj + BLOCK_SIZE : n;
                int k_max = (kk + BLOCK_SIZE < n) ? kk + BLOCK_SIZE : n;

                for (int i = ii; i < i_max; i++)
                {
                    for (int j = jj; j < j_max; j++)
                    {
                        double sum = C[i][j]; // Accumulate
                        for (int k = kk; k < k_max; k++)
                        {
                            sum += A[i][k] * B_transpose[j][k];
                        }
                        C[i][j] = sum;
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int rank, size;
    int n, seed, verbose;
    double start_time;

    MPI_Init(&argc, &argv);
    start_time = MPI_Wtime();

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Parse arguments (only rank 0 will do this, but all ranks need the values)
    if (argc != 4)
    {
        if (rank == 0)
            printf("No arguments provided. Using defaults.\n");

        n = 8000;
        seed = 42;
        verbose = 0;
    }
    else
    {
        n = atoi(argv[1]);
        seed = atoi(argv[2]);
        verbose = atoi(argv[3]);
    }

    int rows_per_process = n / size;
    int remainder = n % size;
    int my_rows = rows_per_process + (rank < remainder ? 1 : 0);

    // Allocate local matrices
    double **my_A = my_rows > 0 ? allocate_matrix(my_rows, n) : NULL;
    double **my_C = my_rows > 0 ? allocate_matrix(my_rows, n) : NULL;
    double **B_transpose = allocate_matrix(n, n); // all ranks need B_transpose

    // Rank 0 only: allocate full matrices, fill them, compute sendcounts/displs
    double **A_full = NULL, **B_full = NULL, **C_full = NULL;
    int *sendcounts = NULL, *displs = NULL;

    if (rank == 0)
    {
        A_full = allocate_matrix(n, n);
        B_full = allocate_matrix(n, n);
        C_full = allocate_matrix(n, n);

        fillArray(A_full, n, seed + 0);
        fillArray(B_full, n, seed + 1);

        // Transpose B for cache efficiency
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                B_transpose[i][j] = B_full[j][i];

        // Print if verbose
        if (verbose && n <= 10)
        {
            print_matrix("A", A_full, n);
            print_matrix("B", B_full, n);
        }

        // Prepare scatter info
        sendcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));

        int offset = 0;

        for (int i = 0; i < size; i++)
        {
            int rows = rows_per_process + (i < remainder ? 1 : 0);
            sendcounts[i] = rows * n;
            displs[i] = offset;
            offset += rows * n;
        }
    }

    // Step 1: Broadcast B_transpose to all ranks
    // for (int i = 0; i < n; i++)
    // MPI_Bcast(B_transpose[i], n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(B_transpose[0], n * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Step 2: Scatter rows of A
    MPI_Scatterv(rank == 0 ? A_full[0] : NULL, sendcounts, displs, MPI_DOUBLE,
                 my_rows > 0 ? my_A[0] : NULL, my_rows * n, MPI_DOUBLE,
                 0, MPI_COMM_WORLD);

    // Step 3: Local multiplication
    if (my_rows > 0)
        multiply_matrices(my_A, B_transpose, my_C, my_rows, n);

    // Step 4: Gather results
    MPI_Gatherv(my_rows > 0 ? my_C[0] : NULL, my_rows * n, MPI_DOUBLE,
                rank == 0 ? C_full[0] : NULL, sendcounts, displs, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    // Rank 0 only: free scatter/gather arrays, print results, free full matrices
    if (rank == 0)
    {
        free(sendcounts);
        free(displs);

        if (verbose && n <= 10)
            print_matrix("C (Result)", C_full, n);

        double checksum = compute_checksum(C_full, n);
        printf("Checksum: %.6f\n", checksum);

        printf("Execution time with %d ranks: %.2f s\n", size, MPI_Wtime() - start_time);

        free_matrix(A_full);
        free_matrix(B_full);
        free_matrix(C_full);
    }

    // Free local matrices for all ranks
    free_matrix(B_transpose);
    free_matrix(my_A);
    free_matrix(my_C);

    MPI_Finalize();
    return 0;
}
