/*
 * Producer-Consumer Application with Circular Bounded Buffer
 * 
 * This program implements a multithreaded producer-consumer pattern using:
 * - Circular bounded buffer for data storage
 * - Semaphores for synchronization (empty slots, full slots)
 * - Mutex for mutual exclusion during buffer access
 * - Priority handling (urgent items processed first)
 * - Throughput and latency metrics
 * - Poison pill technique for graceful termination
 *
 * Compilation: gcc -o producer_consumer producer_consumer.c -pthread -lm
 * Usage: ./producer_consumer <num_producers> <num_consumers> <buffer_size>
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

/* Constants */
#define ITEMS_PER_PRODUCER 20
#define POISON_PILL -1
#define PRIORITY_NORMAL 0
#define PRIORITY_URGENT 1
#define URGENT_PROBABILITY 25  // 25% of items are urgent

/* Buffer item structure */
typedef struct {
    int value;              // Item value (POISON_PILL for termination signal)
    int priority;           // 0 = normal, 1 = urgent
    struct timeval enqueue_time;  // Timestamp when item was enqueued
} BufferItem;

/* Circular buffer structure */
typedef struct {
    BufferItem *items;      // Array of buffer items
    int size;               // Buffer capacity
    int count;              // Current number of items in buffer
    int head;               // Index for next item to remove (consumer)
    int tail;               // Index for next item to insert (producer)
} CircularBuffer;

/* Global variables */
CircularBuffer buffer;
sem_t empty;                // Semaphore to track empty slots
sem_t full;                 // Semaphore to track full slots
pthread_mutex_t mutex;      // Mutex for mutual exclusion

int num_producers;
int num_consumers;
int total_items_produced = 0;
int total_items_consumed = 0;
int poison_pills_consumed = 0;

pthread_mutex_t stats_mutex;  // Mutex for statistics

/* Metrics tracking */
double *latencies;          // Array to store latencies for each consumed item
int latency_count = 0;
struct timeval start_time, end_time;

/* Function prototypes */
void init_buffer(int size);
void destroy_buffer(void);
int insert_item(BufferItem item);
BufferItem remove_item(void);
void *producer(void *param);
void *consumer(void *param);
void validate_inputs(int argc, char *argv[]);
void print_metrics(void);
double get_time_diff(struct timeval start, struct timeval end);

/**
 * Initialize the circular buffer and synchronization primitives
 */
void init_buffer(int size) {
    buffer.items = (BufferItem *)malloc(size * sizeof(BufferItem));
    if (buffer.items == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for buffer\n");
        exit(EXIT_FAILURE);
    }
    
    buffer.size = size;
    buffer.count = 0;
    buffer.head = 0;
    buffer.tail = 0;
    
    // Initialize semaphores
    if (sem_init(&empty, 0, size) != 0) {
        fprintf(stderr, "Error: Failed to initialize empty semaphore\n");
        exit(EXIT_FAILURE);
    }
    
    if (sem_init(&full, 0, 0) != 0) {
        fprintf(stderr, "Error: Failed to initialize full semaphore\n");
        exit(EXIT_FAILURE);
    }
    
    // Initialize mutexes
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize buffer mutex\n");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_mutex_init(&stats_mutex, NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize stats mutex\n");
        exit(EXIT_FAILURE);
    }
    
    // Allocate memory for latency tracking
    int total_items = num_producers * ITEMS_PER_PRODUCER;
    latencies = (double *)malloc(total_items * sizeof(double));
    if (latencies == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for latency tracking\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * Clean up and destroy buffer and synchronization primitives
 */
void destroy_buffer(void) {
    free(buffer.items);
    free(latencies);
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&stats_mutex);
}

/**
 * Insert an item into the buffer (called by producers)
 * Uses priority handling: urgent items inserted before normal items
 */
int insert_item(BufferItem item) {
    // Wait for an empty slot (semaphore blocks if buffer is full)
    sem_wait(&empty);
    
    // Enter critical section
    pthread_mutex_lock(&mutex);
    
    if (item.priority == PRIORITY_URGENT && item.value != POISON_PILL) {
        // Find the correct position to insert urgent item
        // Urgent items should be placed before normal items but maintain FIFO within priority
        
        int insert_pos = buffer.tail;
        int items_to_shift = 0;
        
        // Find how many normal priority items are at the end of the queue
        for (int i = 0; i < buffer.count; i++) {
            int idx = (buffer.head + buffer.count - 1 - i + buffer.size) % buffer.size;
            if (buffer.items[idx].priority == PRIORITY_NORMAL) {
                items_to_shift++;
            } else {
                break;  // Stop when we hit an urgent item or reach the front
            }
        }
        
        // Shift normal items to make room for urgent item
        if (items_to_shift > 0) {
            // Move tail forward
            int new_tail = (buffer.tail + 1) % buffer.size;
            
            // Shift normal items one position toward tail
            for (int i = 0; i < items_to_shift; i++) {
                int from_idx = (buffer.tail - 1 - i + buffer.size) % buffer.size;
                int to_idx = (buffer.tail - i + buffer.size) % buffer.size;
                buffer.items[to_idx] = buffer.items[from_idx];
            }
            
            // Insert urgent item at the correct position
            insert_pos = (buffer.tail - items_to_shift + buffer.size) % buffer.size;
            buffer.items[insert_pos] = item;
            buffer.tail = new_tail;
        } else {
            // No normal items to shift, just insert at tail
            buffer.items[buffer.tail] = item;
            buffer.tail = (buffer.tail + 1) % buffer.size;
        }
    } else {
        // Normal priority or poison pill: insert at tail
        buffer.items[buffer.tail] = item;
        buffer.tail = (buffer.tail + 1) % buffer.size;
    }
    
    buffer.count++;
    
    // Exit critical section
    pthread_mutex_unlock(&mutex);
    
    // Signal that there's a full slot (semaphore signals waiting consumers)
    sem_post(&full);
    
    return 0;
}

/**
 * Remove an item from the buffer (called by consumers)
 * Returns the item at the head of the buffer
 */
BufferItem remove_item(void) {
    // Wait for a full slot (semaphore blocks if buffer is empty)
    sem_wait(&full);
    
    // Enter critical section
    pthread_mutex_lock(&mutex);
    
    // Remove item from head
    BufferItem item = buffer.items[buffer.head];
    buffer.head = (buffer.head + 1) % buffer.size;
    buffer.count--;
    
    // Exit critical section
    pthread_mutex_unlock(&mutex);
    
    // Signal that there's an empty slot (semaphore signals waiting producers)
    sem_post(&empty);
    
    return item;
}

/**
 * Producer thread function
 * Generates random items and inserts them into the buffer
 */
void *producer(void *param) {
    int producer_id = *((int *)param);
    free(param);  // Free the allocated memory for the ID
    
    // Seed random number generator (unique per thread)
    unsigned int seed = time(NULL) + producer_id;
    
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        BufferItem item;
        item.value = rand_r(&seed) % 1000 + 1;  // Random value between 1 and 1000
        
        // 25% chance of being urgent priority
        item.priority = (rand_r(&seed) % 100 < URGENT_PROBABILITY) ? PRIORITY_URGENT : PRIORITY_NORMAL;
        
        // Record enqueue timestamp
        gettimeofday(&item.enqueue_time, NULL);
        
        // Insert item into buffer
        insert_item(item);
        
        // Update statistics
        pthread_mutex_lock(&stats_mutex);
        total_items_produced++;
        pthread_mutex_unlock(&stats_mutex);
        
        // Print production message
        printf("[Producer-%d] Produced item: %d (Priority: %s)\n", 
               producer_id, item.value, 
               item.priority == PRIORITY_URGENT ? "URGENT" : "NORMAL");
    }
    
    printf("[Producer-%d] Finished producing %d items.\n", producer_id, ITEMS_PER_PRODUCER);
    pthread_exit(NULL);
}

/**
 * Consumer thread function
 * Removes items from the buffer and processes them
 */
void *consumer(void *param) {
    int consumer_id = *((int *)param);
    free(param);  // Free the allocated memory for the ID
    
    while (1) {
        // Remove item from buffer
        BufferItem item = remove_item();
        
        // Check for poison pill
        if (item.value == POISON_PILL) {
            pthread_mutex_lock(&stats_mutex);
            poison_pills_consumed++;
            pthread_mutex_unlock(&stats_mutex);
            
            printf("[Consumer-%d] Received poison pill. Terminating.\n", consumer_id);
            break;
        }
        
        // Record dequeue timestamp and calculate latency
        struct timeval dequeue_time;
        gettimeofday(&dequeue_time, NULL);
        double latency = get_time_diff(item.enqueue_time, dequeue_time);
        
        // Update statistics
        pthread_mutex_lock(&stats_mutex);
        latencies[latency_count++] = latency;
        total_items_consumed++;
        pthread_mutex_unlock(&stats_mutex);
        
        // Print consumption message
        printf("[Consumer-%d] Consumed item: %d (Priority: %s, Latency: %.6f sec)\n", 
               consumer_id, item.value,
               item.priority == PRIORITY_URGENT ? "URGENT" : "NORMAL",
               latency);
    }
    
    printf("[Consumer-%d] Finished consuming.\n", consumer_id);
    pthread_exit(NULL);
}

/**
 * Calculate time difference in seconds between two timeval structures
 */
double get_time_diff(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}

/**
 * Calculate and print performance metrics
 */
void print_metrics(void) {
    double total_time = get_time_diff(start_time, end_time);
    
    // Calculate average latency
    double sum_latency = 0.0;
    for (int i = 0; i < latency_count; i++) {
        sum_latency += latencies[i];
    }
    double avg_latency = (latency_count > 0) ? sum_latency / latency_count : 0.0;
    
    // Calculate throughput (items per second)
    double throughput = (total_time > 0) ? total_items_consumed / total_time : 0.0;
    
    printf("\n========== Performance Metrics ==========\n");
    printf("Total items produced: %d\n", total_items_produced);
    printf("Total items consumed: %d\n", total_items_consumed);
    printf("Total execution time: %.6f seconds\n", total_time);
    printf("Average latency: %.6f seconds\n", avg_latency);
    printf("Throughput: %.2f items/second\n", throughput);
    printf("=========================================\n");
}

/**
 * Validate command-line inputs
 */
void validate_inputs(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <num_producers> <num_consumers> <buffer_size>\n", argv[0]);
        fprintf(stderr, "Example: %s 3 2 10\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    num_producers = atoi(argv[1]);
    num_consumers = atoi(argv[2]);
    int buffer_size = atoi(argv[3]);
    
    // Validate inputs
    if (num_producers <= 0) {
        fprintf(stderr, "Error: Number of producers must be positive\n");
        exit(EXIT_FAILURE);
    }
    
    if (num_consumers <= 0) {
        fprintf(stderr, "Error: Number of consumers must be positive\n");
        exit(EXIT_FAILURE);
    }
    
    if (buffer_size <= 0) {
        fprintf(stderr, "Error: Buffer size must be positive\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Configuration: %d producers, %d consumers, buffer size = %d\n", 
           num_producers, num_consumers, buffer_size);
    printf("Each producer will generate %d items.\n", ITEMS_PER_PRODUCER);
    printf("Total items to be produced: %d\n\n", num_producers * ITEMS_PER_PRODUCER);
}

/**
 * Main function
 */
int main(int argc, char *argv[]) {
    // Validate command-line inputs
    validate_inputs(argc, argv);
    
    int buffer_size = atoi(argv[3]);
    
    // Initialize buffer and synchronization primitives
    init_buffer(buffer_size);
    
    // Record start time
    gettimeofday(&start_time, NULL);
    
    // Create producer threads
    pthread_t *producer_threads = (pthread_t *)malloc(num_producers * sizeof(pthread_t));
    if (producer_threads == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for producer threads\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Creating %d producer thread(s)...\n", num_producers);
    for (int i = 0; i < num_producers; i++) {
        int *id = (int *)malloc(sizeof(int));
        if (id == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for producer ID\n");
            exit(EXIT_FAILURE);
        }
        *id = i + 1;
        
        if (pthread_create(&producer_threads[i], NULL, producer, id) != 0) {
            fprintf(stderr, "Error: Failed to create producer thread %d\n", i + 1);
            exit(EXIT_FAILURE);
        }
    }
    
    // Create consumer threads
    pthread_t *consumer_threads = (pthread_t *)malloc(num_consumers * sizeof(pthread_t));
    if (consumer_threads == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for consumer threads\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Creating %d consumer thread(s)...\n\n", num_consumers);
    for (int i = 0; i < num_consumers; i++) {
        int *id = (int *)malloc(sizeof(int));
        if (id == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for consumer ID\n");
            exit(EXIT_FAILURE);
        }
        *id = i + 1;
        
        if (pthread_create(&consumer_threads[i], NULL, consumer, id) != 0) {
            fprintf(stderr, "Error: Failed to create consumer thread %d\n", i + 1);
            exit(EXIT_FAILURE);
        }
    }
    
    // Wait for all producer threads to finish
    printf("Waiting for producers to finish...\n");
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producer_threads[i], NULL);
    }
    printf("All producers finished.\n\n");
    
    // Insert poison pills for consumers
    printf("Inserting %d poison pill(s) for consumers...\n", num_consumers);
    for (int i = 0; i < num_consumers; i++) {
        BufferItem poison;
        poison.value = POISON_PILL;
        poison.priority = PRIORITY_URGENT;  // Poison pills should be processed immediately
        gettimeofday(&poison.enqueue_time, NULL);
        insert_item(poison);
    }
    
    // Wait for all consumer threads to finish
    printf("Waiting for consumers to finish...\n");
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumer_threads[i], NULL);
    }
    printf("All consumers finished.\n\n");
    
    // Record end time
    gettimeofday(&end_time, NULL);
    
    // Print performance metrics
    print_metrics();
    
    // Clean up
    free(producer_threads);
    free(consumer_threads);
    destroy_buffer();
    
    printf("\nProgram completed successfully.\n");
    return 0;
}

