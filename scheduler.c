//
// Created by 006li on 7/10/2024.
//
#include "scheduler.h" // Include the scheduler header file

// Global queues
Queue ready_queue = {NULL, NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER}; // Initialize ready queue
Queue io_queue = {NULL, NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER}; // Initialize IO queue
int file_read_done = 0; // Flag to indicate file read completion

// Enqueue function
void enqueue(Queue *queue, PCB *pcb) {
    pthread_mutex_lock(&queue->mutex); // Lock the queue mutex
    pcb->waiting_time = current_time; // Track the time when the process is enqueued
    if (queue->tail) { // If the queue is not empty
        queue->tail->next = pcb; // Add the PCB to the end of the queue
        pcb->prev = queue->tail; // Set the previous pointer
        queue->tail = pcb; // Update the tail pointer
    } else { // If the queue is empty
        queue->head = queue->tail = pcb; // Set both head and tail to the new PCB
    }
    pthread_cond_signal(&queue->cond); // Signal that a new item is available
    pthread_mutex_unlock(&queue->mutex); // Unlock the queue mutex
}

// Dequeue function
PCB *dequeue(Queue *queue) {
    pthread_mutex_lock(&queue->mutex); // Lock the queue mutex
    while (queue->head == NULL && !file_read_done) { // Wait while the queue is empty and file read is not done
        pthread_cond_wait(&queue->cond, &queue->mutex); // Wait for a condition signal
    }
    if (queue->head == NULL) { // If the queue is still empty
        pthread_mutex_unlock(&queue->mutex); // Unlock the queue mutex
        return NULL; // Return NULL
    }
    PCB *pcb = queue->head; // Get the head PCB
    queue->head = pcb->next; // Move the head pointer to the next PCB
    if (queue->head == NULL) { // If the queue is now empty
        queue->tail = NULL; // Set the tail to NULL
    } else {
        queue->head->prev = NULL; // Clear the previous pointer of the new head
    }
    pcb->next = pcb->prev = NULL; // Clear pointers in the dequeued PCB
    pthread_mutex_unlock(&queue->mutex); // Unlock the queue mutex
    pcb->waiting_time = current_time - pcb->waiting_time; // Update the waiting time
    return pcb; // Return the dequeued PCB
}

// File read thread function
void *file_read_thread(void *arg) {
    char *filename = (char *)arg; // Get the filename from the argument
    FILE *file = fopen(filename, "r"); // Open the file for reading
    if (!file) { // If the file cannot be opened
        perror("Failed to open input file"); // Print an error message
        pthread_exit(NULL); // Exit the thread
    }

    char line[256]; // Buffer to store each line of the file
    while (fgets(line, sizeof(line), file)) { // Read each line of the file
        if (strncmp(line, "proc", 4) == 0) { // If the line starts with "proc"
            PCB *pcb = malloc(sizeof(PCB)); // Allocate memory for a new PCB
            if (pcb == NULL) { // If memory allocation fails
                perror("Failed to allocate memory for PCB"); // Print an error message
                continue; // Skip to the next line
            }
            int burst_count;
            sscanf(line, "proc %d %d", &pcb->priority, &burst_count); // Parse the priority and burst count
            pcb->burst_count = burst_count; // Set the burst count
            pcb->bursts = malloc(burst_count * sizeof(int)); // Allocate memory for the bursts
            if (pcb->bursts == NULL) { // If memory allocation fails
                perror("Failed to allocate memory for bursts"); // Print an error message
                free(pcb); // Free the PCB memory
                continue; // Skip to the next line
            }
            char *token = strtok(line + 5, " "); // Tokenize the line to get burst times
            for (int i = 0; i < burst_count; ++i) { // Loop through each burst
                token = strtok(NULL, " "); // Get the next token
                pcb->bursts[i] = atoi(token); // Convert token to integer and store in bursts
            }
            pcb->current_burst = 0; // Initialize current burst index
            pcb->arrival_time = current_time; // Set the arrival time
            pcb->waiting_time = 0; // Initialize waiting time
            pcb->turnaround_time = 0; // Initialize turnaround time
            pcb->prev = pcb->next = NULL; // Clear pointers
            enqueue(&ready_queue, pcb); // Enqueue the PCB to the ready queue
            printf("Enqueued process with priority %d and %d bursts\n", pcb->priority, burst_count); // Print debug info
        } else if (strncmp(line, "sleep", 5) == 0) { // If the line starts with "sleep"
            int sleep_time;
            sscanf(line, "sleep %d", &sleep_time); // Parse the sleep time
            printf("Sleeping for %d ms\n", sleep_time); // Print debug info
            usleep(sleep_time * 1000); // Sleep for the specified time
            current_time += sleep_time; // Update the current time
        } else if (strncmp(line, "stop", 4) == 0) { // If the line starts with "stop"
            printf("Stopping file read thread\n"); // Print debug info
            break; // Exit the loop
        } else { // If the line is unrecognized
            printf("Unknown command: %s\n", line); // Print debug info
        }
    }

    fclose(file); // Close the file
    pthread_mutex_lock(&ready_queue.mutex); // Lock the ready queue mutex
    file_read_done = 1; // Set the file read done flag
    pthread_cond_broadcast(&ready_queue.cond); // Broadcast to all waiting threads
    pthread_mutex_unlock(&ready_queue.mutex); // Unlock the ready queue mutex
    pthread_exit(NULL); // Exit the thread
}

// CPU scheduler thread function
void *cpu_scheduler_thread(void *arg) {
    SchedulerArgs *args = (SchedulerArgs *)arg; // Get the scheduler arguments from the argument
    if (strcmp(args->algorithm, "FIFO") == 0) { // If the algorithm is FIFO
        run_fifo(); // Run FIFO scheduling
    } else if (strcmp(args->algorithm, "SJF") == 0) { // If the algorithm is SJF
        run_sjf(); // Run SJF scheduling
    } else if (strcmp(args->algorithm, "PR") == 0) { // If the algorithm is PR
        run_pr(); // Run PR scheduling
    } else if (strcmp(args->algorithm, "RR") == 0) { // If the algorithm is RR
        run_rr(args->quantum); // Run RR scheduling with the specified quantum
    }
    pthread_exit(NULL); // Exit the thread
}

// I/O system thread function
void *io_system_thread(void *arg) {
    while (1) { // Infinite loop
        PCB *pcb = dequeue(&io_queue); // Dequeue a PCB from the IO queue
        if (!pcb) { // If no PCB is dequeued
            if (file_read_done && ready_queue.head == NULL && io_queue.head == NULL) { // Check for termination condition
                break; // Exit the loop
            }
            continue; // Continue to the next iteration
        }

        // Simulate I/O burst
        usleep(pcb->bursts[pcb->current_burst] * 1000); // Sleep for the burst time
        current_time += pcb->bursts[pcb->current_burst]; // Update the current time

        pcb->current_burst++; // Increment the current burst index
        if (pcb->current_burst < pcb->burst_count) { // If there are more bursts
            enqueue(&ready_queue, pcb); // Enqueue the PCB back to the ready queue
            printf("Processed I/O for process\n"); // Print debug info
        } else {
            // Process finished during I/O
            printf("Process finished during I/O with priority %d\n", pcb->priority); // Print debug info
            pcb->turnaround_time = current_time - pcb->arrival_time; // Calculate turnaround time
            total_turnaround_time += pcb->turnaround_time; // Update total turnaround time
            total_waiting_time += pcb->waiting_time; // Update total waiting time
            process_count++; // Increment process count
            free(pcb->bursts); // Free the bursts array
            free(pcb); // Free the PCB
        }
    }

    pthread_exit(NULL); // Exit the thread
}

// FIFO scheduling function
void run_fifo() {
    while (1) { // Infinite loop
        PCB *pcb = dequeue(&ready_queue); // Dequeue a PCB from the ready queue
        if (!pcb) { // If no PCB is dequeued
            if (file_read_done && ready_queue.head == NULL && io_queue.head == NULL) { // Check for termination condition
                break; // Exit the loop
            }
            continue; // Continue to the next iteration
        }
        int burst_time = pcb->bursts[pcb->current_burst]; // Get the burst time of the current burst
        printf("Running process with priority %d for %d ms\n", pcb->priority, burst_time); // Print debug info
        usleep(burst_time * 1000); // Sleep for the burst time
        busy_time += burst_time; // Update the busy time
        current_time += burst_time; // Update the current time
        pcb->current_burst++; // Increment the current burst index
        if (pcb->current_burst < pcb->burst_count) { // If there are more bursts
            enqueue(&io_queue, pcb); // Enqueue the PCB to the IO queue
        } else {
            printf("Process finished with priority %d\n", pcb->priority); // Print debug info
            pcb->turnaround_time = current_time - pcb->arrival_time; // Calculate turnaround time
            total_turnaround_time += pcb->turnaround_time; // Update total turnaround time
            total_waiting_time += pcb->waiting_time; // Update total waiting time
            process_count++; // Increment process count
            free(pcb->bursts); // Free the bursts array
            free(pcb); // Free the PCB
        }
    }
}

// SJF scheduling function
void run_sjf() {
    while (1) { // Infinite loop
        PCB *shortest_pcb = NULL; // Pointer to the shortest PCB
        PCB *current = ready_queue.head; // Pointer to the current PCB in the queue

        pthread_mutex_lock(&ready_queue.mutex); // Lock the ready queue mutex
        while (current != NULL) { // Iterate through the queue
            if (shortest_pcb == NULL || current->bursts[current->current_burst] < shortest_pcb->bursts[shortest_pcb->current_burst]) { // Find the shortest burst
                shortest_pcb = current; // Update the shortest PCB
            }
            current = current->next; // Move to the next PCB
        }

        if (shortest_pcb != NULL) { // If a shortest PCB is found
            if (shortest_pcb->prev != NULL) { // If the shortest PCB is not the head
                shortest_pcb->prev->next = shortest_pcb->next; // Remove it from the queue
            } else {
                ready_queue.head = shortest_pcb->next; // Update the head pointer
            }

            if (shortest_pcb->next != NULL) { // If the shortest PCB is not the tail
                shortest_pcb->next->prev = shortest_pcb->prev; // Remove it from the queue
            } else {
                ready_queue.tail = shortest_pcb->prev; // Update the tail pointer
            }
            shortest_pcb->next = shortest_pcb->prev = NULL; // Clear the pointers
        }
        pthread_mutex_unlock(&ready_queue.mutex); // Unlock the ready queue mutex

        if (!shortest_pcb) { // If no shortest PCB is found
            if (file_read_done && ready_queue.head == NULL && io_queue.head == NULL) { // Check for termination condition
                break; // Exit the loop
            }
            continue; // Continue to the next iteration
        }

        int burst_time = shortest_pcb->bursts[shortest_pcb->current_burst]; // Get the burst time of the shortest PCB
        usleep(burst_time * 1000); // Sleep for the burst time
        busy_time += burst_time; // Update the busy time
        current_time += burst_time; // Update the current time
        shortest_pcb->current_burst++; // Increment the current burst index
        if (shortest_pcb->current_burst < shortest_pcb->burst_count) { // If there are more bursts
            enqueue(&io_queue, shortest_pcb); // Enqueue the PCB to the IO queue
        } else {
            // Process finished
            printf("Process finished with priority %d\n", shortest_pcb->priority); // Print debug info
            shortest_pcb->turnaround_time = current_time - shortest_pcb->arrival_time; // Calculate turnaround time
            total_turnaround_time += shortest_pcb->turnaround_time; // Update total turnaround time
            total_waiting_time += shortest_pcb->waiting_time; // Update total waiting time
            process_count++; // Increment process count
            free(shortest_pcb->bursts); // Free the bursts array
            free(shortest_pcb); // Free the PCB
        }
    }
}

// Priority scheduling function
void run_pr() {
    while (1) { // Infinite loop
        PCB *highest_priority_pcb = NULL; // Pointer to the highest priority PCB
        PCB *current = ready_queue.head; // Pointer to the current PCB in the queue

        pthread_mutex_lock(&ready_queue.mutex); // Lock the ready queue mutex
        while (current != NULL) { // Iterate through the queue
            if (highest_priority_pcb == NULL || current->priority > highest_priority_pcb->priority) { // Find the highest priority
                highest_priority_pcb = current; // Update the highest priority PCB
            }
            current = current->next; // Move to the next PCB
        }

        if (highest_priority_pcb != NULL) { // If a highest priority PCB is found
            if (highest_priority_pcb->prev != NULL) { // If the highest priority PCB is not the head
                highest_priority_pcb->prev->next = highest_priority_pcb->next; // Remove it from the queue
            } else {
                ready_queue.head = highest_priority_pcb->next; // Update the head pointer
            }

            if (highest_priority_pcb->next != NULL) { // If the highest priority PCB is not the tail
                highest_priority_pcb->next->prev = highest_priority_pcb->prev; // Remove it from the queue
            } else {
                ready_queue.tail = highest_priority_pcb->prev; // Update the tail pointer
            }
            highest_priority_pcb->next = highest_priority_pcb->prev = NULL; // Clear the pointers
        }
        pthread_mutex_unlock(&ready_queue.mutex); // Unlock the ready queue mutex

        if (!highest_priority_pcb) { // If no highest priority PCB is found
            if (file_read_done && ready_queue.head == NULL && io_queue.head == NULL) { // Check for termination condition
                break; // Exit the loop
            }
            continue; // Continue to the next iteration
        }

        int burst_time = highest_priority_pcb->bursts[highest_priority_pcb->current_burst]; // Get the burst time of the highest priority PCB
        usleep(burst_time * 1000); // Sleep for the burst time
        busy_time += burst_time; // Update the busy time
        current_time += burst_time; // Update the current time
        highest_priority_pcb->current_burst++; // Increment the current burst index
        if (highest_priority_pcb->current_burst < highest_priority_pcb->burst_count) { // If there are more bursts
            enqueue(&io_queue, highest_priority_pcb); // Enqueue the PCB to the IO queue
        } else {
            // Process finished
            printf("Process finished with priority %d\n", highest_priority_pcb->priority); // Print debug info
            highest_priority_pcb->turnaround_time = current_time - highest_priority_pcb->arrival_time; // Calculate turnaround time
            total_turnaround_time += highest_priority_pcb->turnaround_time; // Update total turnaround time
            total_waiting_time += highest_priority_pcb->waiting_time; // Update total waiting time
            process_count++; // Increment process count
            free(highest_priority_pcb->bursts); // Free the bursts array
            free(highest_priority_pcb); // Free the PCB
        }
    }
}

// Round Robin scheduling function
void run_rr(int quantum) {
    while (1) { // Infinite loop
        PCB *pcb = dequeue(&ready_queue); // Dequeue a PCB from the ready queue
        if (!pcb) { // If no PCB is dequeued
            if (file_read_done && ready_queue.head == NULL && io_queue.head == NULL) { // Check for termination condition
                break; // Exit the loop
            }
            continue; // Continue to the next iteration
        }

        int burst_time = pcb->bursts[pcb->current_burst]; // Get the burst time of the current burst
        if (burst_time > quantum) { // If the burst time is greater than the quantum
            printf("Running process with priority %d for quantum %d ms\n", pcb->priority, quantum); // Print debug info
            usleep(quantum * 1000); // Sleep for the quantum time
            pcb->bursts[pcb->current_burst] -= quantum; // Decrement the burst time
            busy_time += quantum; // Update the busy time
            current_time += quantum; // Update the current time
            enqueue(&ready_queue, pcb); // Enqueue the PCB back to the ready queue
        } else {
            printf("Running process with priority %d for %d ms\n", pcb->priority, burst_time); // Print debug info
            usleep(burst_time * 1000); // Sleep for the burst time
            busy_time += burst_time; // Update the busy time
            current_time += burst_time; // Update the current time
            pcb->current_burst++; // Increment the current burst index
            if (pcb->current_burst < pcb->burst_count) { // If there are more bursts
                enqueue(&io_queue, pcb); // Enqueue the PCB to the IO queue
            } else {
                printf("Process finished with priority %d\n", pcb->priority); // Print debug info
                pcb->turnaround_time = current_time - pcb->arrival_time; // Calculate turnaround time
                total_turnaround_time += pcb->turnaround_time; // Update total turnaround time
                total_waiting_time += pcb->waiting_time; // Update total waiting time
                process_count++; // Increment process count
                free(pcb->bursts); // Free the bursts array
                free(pcb); // Free the PCB
            }
        }
    }
}
