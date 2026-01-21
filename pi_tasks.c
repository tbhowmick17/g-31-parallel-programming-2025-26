#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

unsigned long my_rand(unsigned long *state) {
    *state = (*state * 0x2545F4914F6CDD1DULL);
    return *state;
}

double compute_pi(long steps) {
    double sum = 0.0;
    double step = 1.0 / (double)steps;

    for (long i = 0; i < steps; ++i) {
        double x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }
    return sum * step;
}

int main(int argc, char *argv[]) {

    if (argc != 6) {
        fprintf(stderr,
            "Usage: %s num_tasks num_threads lower upper seed\n",
            argv[0]);
        return 1;
    }

    int num_tasks = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    long lower = atol(argv[3]);
    long upper = atol(argv[4]);
    unsigned long seed = strtoul(argv[5], NULL, 10);

    omp_set_num_threads(num_threads);

    double pi_sum = 0.0;
    int tasks_spawned = 0;  // tasks that have been created so far
    int to_spawn = 1;       // tasks to spawn in the next batch

    int *tasks_per_thread = (int*)calloc(num_threads, sizeof(int));

    double start_time = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp single
        {
            // Keep spawning batches until we reach num_tasks
            while (tasks_spawned < num_tasks) {
                
                int batch_size = to_spawn;
                to_spawn = 0;  // Reset for next iteration
                
                // Spawn the current batch
                for (int i = 0; i < batch_size; ++i) {
                    
                    #pragma omp task
                    {
                        int my_id;
                        unsigned long my_seed;
                        int valid = 0;
                        
                        // Atomically claim a task ID
                        #pragma omp critical
                        {
                            if (tasks_spawned < num_tasks) {
                                my_id = tasks_spawned++;
                                valid = 1;
                            }
                        }
                        
                        if (valid) {
                            my_seed = seed + my_id;
                            
                            // Random precision
                            long steps = lower + (my_rand(&my_seed) % (upper - lower + 1));
                            
                            // Compute pi
                            double pi = compute_pi(steps);
                            
                            #pragma omp atomic
                            pi_sum += pi;
                            
                            #pragma omp atomic
                            tasks_per_thread[omp_get_thread_num()]++;
                            
                            // Random children (1-4)
                            int num_children = 1 + (my_rand(&my_seed) % 4);
                            
                            // Add to next batch
                            #pragma omp atomic
                            to_spawn += num_children;
                        }
                    }
                }
                
                // wait for this batch to complete before spawning next batch
                #pragma omp taskwait
            }
        }
    }

    double end_time = omp_get_wtime();

    printf("Average pi: %.10f\n", pi_sum / num_tasks);
    
    for (int i = 0; i < num_threads; ++i)
        printf("Thread %d computed %d tasks\n", i, tasks_per_thread[i]);
    
    printf("Execution took %.4f s\n", end_time - start_time);

    free(tasks_per_thread);
    return 0;
}