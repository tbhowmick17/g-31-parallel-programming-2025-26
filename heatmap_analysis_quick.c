// Author: Trisha Bhowmick

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>


unsigned long hash(unsigned long x) {
    x ^= (x >> 21);
    x ^= (x >> 13);
    x *= 2654435761UL;
    x *= 2654435761UL;
    x ^= (x >> 17);
    return x;
}

unsigned concatenate(unsigned x, unsigned y) {
    unsigned pow = 10;
    while (y >= pow)
    pow *= 10;
    return x * pow + y;
}

unsigned long my_rand(unsigned long* state, unsigned long lower, unsigned long upper) {
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    unsigned long result = (*state * 0x2545F4914F6CDD1DULL);
    unsigned long range = (upper > lower) ? (upper - lower) : 0UL;
    return (range > 0) ? (result % range + lower) : lower;
}

int main(int argc, char **argv) {

    if (argc != 10) {
        fprintf(stderr, "Usage: %s <num_rows> <num_cols> <num_threads> <num_iterations> <lower_bound> <upper_bound> <seed> <output_file> <hash_seed>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const int columns = atoi(argv[1]);
    const int rows = atoi(argv[2]);
    const unsigned long seed = strtoul(argv[3], NULL, 10);
    const unsigned long lower = strtoul(argv[4], NULL, 10);
    const unsigned long upper = strtoul(argv[5], NULL, 10);
    const int window_height = atoi(argv[6]);
    const int verbose = atoi(argv[7]);
    const int num_threads = atoi(argv[8]);
    const int work_factor = atoi(argv[9]);

    omp_set_num_threads(num_threads);

    const double start_time = omp_get_wtime();

    unsigned long *A = (unsigned long*)malloc(rows * columns * sizeof(unsigned long));
    if (!A) {
        fprintf(stderr, "Memory allocation failed for matrix A.\n");
        return EXIT_FAILURE;
    }

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            unsigned long s = seed * concatenate(i, j);
            unsigned long v = my_rand(&s, lower, upper);

            for (int w = 0; w < work_factor; w++) {
                v = hash(v);
            }

            A[(size_t)(i * columns + j)] = v;

        }
    }

    int found_empty_row = 0;
    int empty_row_index = -1;

    #pragma omp parallel for shared(found_empty_row, empty_row_index) schedule(static)
    for (int i = 0; i < rows; i++) {
        if (found_empty_row) continue;

        int has_hotspot = 0;

        for (int j = 0; j < columns; j++) {
            unsigned long center = A[(size_t)(i * columns + j)];

            if ( i > 0 && A[(size_t)((i - 1) * columns + j)] >= center) continue;
            if ( i < rows - 1 && A[(size_t)((i + 1) * columns + j)] >= center) continue;
            if ( j > 0 && A[(size_t)(i * columns + j - 1)] >= center) continue;
            if ( j < columns - 1 && A[(size_t)(i * columns + j + 1)] >= center) continue;

            has_hotspot = 1;
            break;
        }
        if (!has_hotspot) {
            #pragma omp atomic write
            found_empty_row = 1;

            #pragma omp critical
            {
                if (empty_row_index == -1) {
                    empty_row_index = i;
                }
            }
        }
    }
    
    if (found_empty_row) {
        printf("No hotspots found in row %d\n", empty_row_index);
        printf("Early Exit\n");
    } else {
        printf("All rows contain atleast one hotspot.\n");
    }

    const double end_time = omp_get_wtime();

    printf("Execution Time: %f seconds\n", end_time - start_time);

    free(A);
    return EXIT_SUCCESS;
}
