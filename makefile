# Compiler
CC = gcc

# Compiler flags
CFLAGS = -O3 -fopenmp -std=c11 -Wall

# Targets
all: heatmap_analysis heatmap_analysis_quick pi_tasks

heatmap_analysis: heatmap_analysis.c
	$(CC) $(CFLAGS) heatmap_analysis.c -o heatmap_analysis

heatmap_analysis_quick: heatmap_analysis_quick.c
	$(CC) $(CFLAGS) heatmap_analysis_quick.c -o heatmap_analysis_quick

pi_tasks: pi_tasks.c
	$(CC) $(CFLAGS) pi_tasks.c -o pi_tasks

clean:
	rm -f heatmap_analysis heatmap_analysis_quick pi_tasks