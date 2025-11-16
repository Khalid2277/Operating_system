#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>

/* Constants */
#define ITEMS_PER_PRODUCER 20
#define POISON_PILL -1

/* Buffer item structure */
typedef struct {
    int value;
    int priority;  // 0 = normal, 1 = urgent (bonus feature)
    struct timeval timestamp;  // for latency calculation (bonus feature)
} item;

/* Circular buffer */
item *buffer;
int buffer_size;
int in = 0;   // tail index (where producer inserts)
int out = 0;  // head index (where consumer removes)

/* Semaphores  */
sem_t mutex;  // initialized to 1 (mutual exclusion)
sem_t empty;  // initialized to n (empty slots)
sem_t full;   // initialized to 0 (full slots)

/* Global variables */
int num_producers;
int num_consumers;
int total_produced = 0;
int total_consumed = 0;

/* Metrics for bonus feature */
struct timeval start_time, end_time;
double total_latency = 0.0;
pthread_mutex_t stats_lock;

/* Function prototypes */
void *producer(void *param);
void *consumer(void *param);
void insert_item(item next_produced);
item remove_item(void);

/**
 * Producer thread implementation
 */
void *producer(void *param) {
    int id = *((int *)param);
    free(param);
    
    unsigned int seed = time(NULL) + id;
    
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        item next_produced;
        
        /* produce an item in next_produced */
        next_produced.value = rand_r(&seed) % 1000 + 1;
        next_produced.priority = (rand_r(&seed) % 100 < 25) ? 1 : 0;  // 25% urgent
        gettimeofday(&next_produced.timestamp, NULL);
        
        /* insert item into buffer */
        insert_item(next_produced);
        
        pthread_mutex_lock(&stats_lock);
        total_produced++;
        pthread_mutex_unlock(&stats_lock);
        
        printf("[P%d] Produced: %d (Priority: %s)\n", 
               id, next_produced.value,
               next_produced.priority ? "URGENT" : "NORMAL");
    }
    
    printf("[P%d] Finished\n", id);
    pthread_exit(NULL);
}

/**
 * Consumer thread implementation
 */
void *consumer(void *param) {
    int id = *((int *)param);
    free(param);
    
    while (1) {
        item next_consumed;
        
        /* remove an item from buffer to next_consumed */
        next_consumed = remove_item();
        
        /* check for poison pill */
        if (next_consumed.value == POISON_PILL) {
            printf("[C%d] Received poison pill. Terminating.\n", id);
            break;
        }
        
        /* calculate latency (bonus feature) */
        struct timeval now;
        gettimeofday(&now, NULL);
        double latency = (now.tv_sec - next_consumed.timestamp.tv_sec) +
                        (now.tv_usec - next_consumed.timestamp.tv_usec) / 1000000.0;
        
        pthread_mutex_lock(&stats_lock);
        total_consumed++;
        total_latency += latency;
        pthread_mutex_unlock(&stats_lock);
        
        /* consume the item in next_consumed */
        printf("[C%d] Consumed: %d (Priority: %s, Latency: %.6f sec)\n",
               id, next_consumed.value,
               next_consumed.priority ? "URGENT" : "NORMAL",
               latency);
    }
    
    pthread_exit(NULL);
}

/**
 * Insert item into buffer
 */
void insert_item(item next_produced) {
    sem_wait(&empty);  // wait for empty slot
    sem_wait(&mutex);  // enter critical section
    
    /* Critical Section - Add next_produced to the buffer */
    buffer[in] = next_produced;
    in = (in + 1) % buffer_size;  // move tail forward (circular)
    
    sem_post(&mutex);  // exit critical section
    sem_post(&full);   // signal full slot
}

/**
 * Remove item from buffer
 */
item remove_item(void) {
    sem_wait(&full);   // wait for full slot
    sem_wait(&mutex);  // enter critical section
    
    /* Critical Section - Remove item from buffer */
    item next_consumed = buffer[out];
    out = (out + 1) % buffer_size;
    
    sem_post(&mutex);  // exit critical section
    sem_post(&empty);  // signal empty slot
    
    return next_consumed;
}

/**
 * Main function
 */
int main(int argc, char *argv[]) {
    /* Validate input */
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <num_producers> <num_consumers> <buffer_size>\n", argv[0]);
        return 1;
    }
    
    num_producers = atoi(argv[1]);
    num_consumers = atoi(argv[2]);
    buffer_size = atoi(argv[3]);
    
    if (num_producers <= 0 || num_consumers <= 0 || buffer_size <= 0) {
        fprintf(stderr, "Error: All arguments must be positive integers\n");
        return 1;
    }
    
    printf("Configuration: %d producers, %d consumers, buffer size = %d\n",
           num_producers, num_consumers, buffer_size);
    printf("Each producer generates %d items\n\n", ITEMS_PER_PRODUCER);
    
    /* Allocate buffer */
    buffer = (item *)malloc(buffer_size * sizeof(item));
    if (buffer == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    sem_init(&mutex, 0, 1);           // binary semaphore for mutual exclusion
    sem_init(&empty, 0, buffer_size); // counting semaphore for empty slots
    sem_init(&full, 0, 0);            // counting semaphore for full slots
    
    /* Initialize statistics mutex */
    pthread_mutex_init(&stats_lock, NULL);
    
    /* Record start time */
    gettimeofday(&start_time, NULL);
    
    /* Create producer threads */
    pthread_t *producers = (pthread_t *)malloc(num_producers * sizeof(pthread_t));
    printf("Creating %d producer thread(s)...\n", num_producers);
    for (int i = 0; i < num_producers; i++) {
        int *id = (int *)malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&producers[i], NULL, producer, id);
    }
    
    /* Create consumer threads */
    pthread_t *consumers = (pthread_t *)malloc(num_consumers * sizeof(pthread_t));
    printf("Creating %d consumer thread(s)...\n\n", num_consumers);
    for (int i = 0; i < num_consumers; i++) {
        int *id = (int *)malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&consumers[i], NULL, consumer, id);
    }
    
    /* Wait for all producers to finish */
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }
    printf("\nAll producers finished.\n");
    
    /* Insert poison pills for consumers */
    printf("Inserting %d poison pill(s)...\n", num_consumers);
    for (int i = 0; i < num_consumers; i++) {
        item poison;
        poison.value = POISON_PILL;
        poison.priority = 1;  // urgent priority
        gettimeofday(&poison.timestamp, NULL);
        insert_item(poison);
    }
    
    /* Wait for all consumers to finish */
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }
    printf("All consumers finished.\n\n");
    
    /* Record end time */
    gettimeofday(&end_time, NULL);
    
    /* Calculate and display metrics (bonus feature) */
    double total_time = (end_time.tv_sec - start_time.tv_sec) +
                       (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    double avg_latency = (total_consumed > 0) ? total_latency / total_consumed : 0.0;
    double throughput = (total_time > 0) ? total_consumed / total_time : 0.0;
    
    printf("========== Performance Metrics ==========\n");
    printf("Total items produced: %d\n", total_produced);
    printf("Total items consumed: %d\n", total_consumed);
    printf("Total execution time: %.6f seconds\n", total_time);
    printf("Average latency: %.6f seconds\n", avg_latency);
    printf("Throughput: %.2f items/second\n", throughput);
    printf("=========================================\n");
    
    /* Cleanup */
    free(buffer);
    free(producers);
    free(consumers);
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&stats_lock);
    
    printf("\nProgram completed successfully.\n");
    return 0;
}

