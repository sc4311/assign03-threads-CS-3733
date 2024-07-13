//
// Created by 006li on 7/9/2024.
//
#include "scheduler.h" // Include the scheduler header file

// Global variables to store command line arguments
char *algorithm = NULL; // Pointer to the scheduling algorithm
char *input_file = NULL; // Pointer to the input file name
int quantum = 0; // Time quantum for Round Robin scheduling

// Metrics
int total_time = 0; // Total time taken
int busy_time = 0; // Time when CPU is busy
int process_count = 0; // Number of processes
int total_turnaround_time = 0; // Sum of turnaround times of all processes
int total_waiting_time = 0; // Sum of waiting times of all processes
int current_time = 0; // Current time

// Function to parse command line arguments
void parse_arguments(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) { // Iterate over each argument
        if (strcmp(argv[i], "-alg") == 0 && i + 1 < argc) { // Check for algorithm flag
            algorithm = argv[i + 1]; // Set the algorithm
            i++; // Skip next argument
        } else if (strcmp(argv[i], "-input") == 0 && i + 1 < argc) { // Check for input file flag
            input_file = argv[i + 1]; // Set the input file
            i++; // Skip next argument
        } else if (strcmp(argv[i], "-quantum") == 0 && i + 1 < argc) { // Check for quantum flag
            quantum = atoi(argv[i + 1]); // Set the quantum value
            i++; // Skip next argument
        }
    }

    // Check for required arguments and valid values
    if (algorithm == NULL || input_file == NULL ||
        (strcmp(algorithm, "RR") == 0 && quantum == 0)) {
        fprintf(stderr, "Usage: %s -alg [FIFO|SJF|PR|RR] [-quantum [integer (ms)]] -input [file name]\n", argv[0]);
        exit(EXIT_FAILURE); // Exit if arguments are not valid
    }
}

// Main function
int main(int argc, char *argv[]) {
    parse_arguments(argc, argv); // Parse command line arguments

    SchedulerArgs scheduler_args = {algorithm, quantum}; // Set scheduler arguments

    pthread_t file_thread, cpu_thread, io_thread; // Declare thread variables
    pthread_create(&file_thread, NULL, file_read_thread, (void *)input_file); // Create file reading thread
    pthread_create(&cpu_thread, NULL, cpu_scheduler_thread, (void *)&scheduler_args); // Create CPU scheduling thread
    pthread_create(&io_thread, NULL, io_system_thread, NULL); // Create I/O system thread

    pthread_join(file_thread, NULL); // Wait for file thread to finish
    pthread_join(cpu_thread, NULL); // Wait for CPU thread to finish
    pthread_join(io_thread, NULL); // Wait for I/O thread to finish

    // Output performance metrics
    total_time = current_time; // Set total time to current time
    float cpu_utilization = (float)busy_time / total_time * 100; // Calculate CPU utilization
    float throughput = (float)process_count / total_time; // Calculate throughput (processes per ms)
    float avg_turnaround_time = (float)total_turnaround_time / process_count; // Calculate average turnaround time
    float avg_waiting_time = (float)total_waiting_time / process_count; // Calculate average waiting time

    // Print metrics
    printf("Input File Name              : %s\n", input_file);
    printf("CPU Scheduling Alg           : %s\n", algorithm);
    if (strcmp(algorithm, "RR") == 0) {
        printf("Quantum                      : %d ms\n", quantum);
    }
    printf("CPU utilization              : %.3f%%\n", cpu_utilization);
    printf("Throughput                   : %.3f processes / ms\n", throughput);
    printf("Avg. Turnaround time         : %.1fms\n", avg_turnaround_time);
    printf("Avg. Waiting time in R queue : %.1fms\n", avg_waiting_time);

    // Debug prints to verify calculations
    printf("Total time: %d ms\n", total_time);
    printf("Busy time: %d ms\n", busy_time);
    printf("Total turnaround time: %d ms\n", total_turnaround_time);
    printf("Total waiting time: %d ms\n", total_waiting_time);
    printf("Process count: %d\n", process_count);

    return 0; // Return success
}
