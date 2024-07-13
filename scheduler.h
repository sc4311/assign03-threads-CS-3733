//
// Created by 006li on 7/10/2024.
//
#ifndef SCHEDULER_H // If not defined, define SCHEDULER_H to prevent multiple inclusions
#define SCHEDULER_H // Define SCHEDULER_H

#include <stdio.h> // Include standard I/O library
#include <stdlib.h> // Include standard library
#include <string.h> // Include string handling library
#include <pthread.h> // Include pthread library for threading
#include <unistd.h> // Include POSIX standard library

// Define the PCB (Process Control Block) structure
typedef struct PCB {
    int priority; // Process priority
    int burst_count; // Number of bursts
    int *bursts; // Array of bursts
    int current_burst; // Index of the current burst
    int arrival_time; // Arrival time of the process
    int waiting_time; // Waiting time of the process
    int turnaround_time; // Turnaround time of the process
    struct PCB *next; // Pointer to the next PCB in the queue
    struct PCB *prev; // Pointer to the previous PCB in the queue
} PCB;

// Define the Queue structure
typedef struct Queue {
    PCB *head; // Pointer to the head of the queue
    PCB *tail; // Pointer to the tail of the queue
    pthread_mutex_t mutex; // Mutex for thread synchronization
    pthread_cond_t cond; // Condition variable for thread synchronization
} Queue;

// Define the SchedulerArgs structure
typedef struct SchedulerArgs {
    char *algorithm; // Scheduling algorithm
    int quantum; // Time quantum for round-robin scheduling
} SchedulerArgs;

extern Queue ready_queue; // Declare the ready queue as an external variable
extern Queue io_queue; // Declare the IO queue as an external variable
extern int file_read_done; // Declare the file read done flag as an external variable

// Declare global metrics variables as external variables
extern int total_time;
extern int busy_time;
extern int process_count;
extern int total_turnaround_time;
extern int total_waiting_time;
extern int current_time;

void enqueue(Queue *queue, PCB *pcb); // Function prototype for enqueueing a PCB to a queue
PCB *dequeue(Queue *queue); // Function prototype for dequeueing a PCB from a queue
void *file_read_thread(void *arg); // Function prototype for the file read thread
void *cpu_scheduler_thread(void *arg); // Function prototype for the CPU scheduler thread
void *io_system_thread(void *arg); // Function prototype for the IO system thread

void run_fifo(); // Function prototype for FIFO scheduling algorithm
void run_sjf(); // Function prototype for SJF scheduling algorithm
void run_pr(); // Function prototype for priority scheduling algorithm
void run_rr(int quantum); // Function prototype for round-robin scheduling algorithm

#endif // SCHEDULER_H // End of include guard
