# Multithreaded Producer-Consumer Application

## Project Overview

This project implements a multithreaded Producer-Consumer application using a circular bounded buffer. The application demonstrates proper synchronization mechanisms using POSIX semaphores and mutexes to ensure data integrity and prevent race conditions.

## Features

### Core Features
- **Circular Bounded Buffer**: Fixed-size buffer with configurable capacity
- **Multiple Producer Threads**: Configurable number of producers generating random data
- **Multiple Consumer Threads**: Configurable number of consumers processing data
- **Semaphore-based Synchronization**: Uses semaphores to manage buffer slots (no busy-waiting)
- **Mutex Protection**: Ensures mutual exclusion during buffer access
- **Graceful Termination**: Implements poison pill technique for clean shutdown
- **Input Validation**: Comprehensive error handling and input validation

### Bonus Features
- **Priority Handling (5%)**: Items have priority levels (0=normal, 1=urgent). Consumers always process urgent items first while maintaining FIFO order within each priority level. Approximately 25% of items are marked as urgent.
- **Throughput and Latency Metrics (5%)**: Tracks enqueue and dequeue timestamps for each item, reports average latency and throughput (items per second).

## Requirements

- **Operating System**: **Linux ONLY** (Ubuntu, Debian, Fedora, CentOS, etc.)
  - ⚠️ **NOT compatible with macOS** - macOS has deprecated POSIX semaphore functions
  - ⚠️ **NOT compatible with Windows** - requires POSIX threads
- **Compiler**: GCC with pthread support
- **Libraries**: POSIX threads (pthread), semaphores, math library

## Compilation

### Option 1: Using Makefile (Recommended)

```bash
make
```

This will compile the program with appropriate flags.

### Option 2: Manual Compilation

```bash
gcc -o producer_consumer producer_consumer.c -pthread -lm
```

**Flags explained**:
- `-pthread`: Links the POSIX thread library
- `-lm`: Links the math library (required for metric calculations)
- `-Wall -Wextra`: Enable all warnings (optional but recommended)

### Additional Make Commands

```bash
make clean       # Remove compiled binary
make test        # Compile and run standard test
make test-all    # Run multiple test configurations
make help        # Show all available commands
```

## Usage

```bash
./producer_consumer <num_producers> <num_consumers> <buffer_size>
```

**Parameters**:
- `num_producers`: Number of producer threads (must be > 0)
- `num_consumers`: Number of consumer threads (must be > 0)
- `buffer_size`: Size of the circular buffer (must be > 0)

**Note**: Each producer generates exactly 20 items.

## Example Test Cases

### Test Case 1: Standard Configuration
```bash
./producer_consumer 3 2 10
```
- 3 producer threads
- 2 consumer threads
- Buffer size of 10 slots
- Total items: 3 × 20 = 60 items

**Expected Output**:
```
Configuration: 3 producers, 2 consumers, buffer size = 10
Each producer will generate 20 items.
Total items to be produced: 60

Creating 3 producer thread(s)...
Creating 2 consumer thread(s)...

[Producer-1] Produced item: 42 (Priority: NORMAL)
[Producer-2] Produced item: 17 (Priority: URGENT)
[Consumer-1] Consumed item: 17 (Priority: URGENT, Latency: 0.000012 sec)
[Producer-3] Produced item: 88 (Priority: NORMAL)
[Consumer-2] Consumed item: 42 (Priority: NORMAL, Latency: 0.000015 sec)
...
[Producer-1] Finished producing 20 items.
[Producer-2] Finished producing 20 items.
[Producer-3] Finished producing 20 items.
All producers finished.

Inserting 2 poison pill(s) for consumers...
Waiting for consumers to finish...
[Consumer-1] Received poison pill. Terminating.
[Consumer-1] Finished consuming.
[Consumer-2] Received poison pill. Terminating.
[Consumer-2] Finished consuming.
All consumers finished.

========== Performance Metrics ==========
Total items produced: 60
Total items consumed: 60
Total execution time: 0.002345 seconds
Average latency: 0.000018 seconds
Throughput: 25584.11 items/second
=========================================

Program completed successfully.
```

### Test Case 2: Small Buffer (High Contention)
```bash
./producer_consumer 5 3 2
```
- 5 producer threads
- 3 consumer threads
- Small buffer size of 2 slots (high contention scenario)
- Total items: 5 × 20 = 100 items

This test case demonstrates:
- Frequent blocking when buffer is full/empty
- Higher average latency due to contention
- Lower throughput compared to larger buffer

### Test Case 3: Large Buffer (Low Contention)
```bash
./producer_consumer 5 3 32
```
- 5 producer threads
- 3 consumer threads
- Large buffer size of 32 slots
- Total items: 5 × 20 = 100 items

This test case demonstrates:
- Less blocking
- Lower average latency
- Higher throughput

### Test Case 4: Many Threads
```bash
./producer_consumer 10 10 15
```
- 10 producer threads
- 10 consumer threads
- Buffer size of 15 slots
- Total items: 10 × 20 = 200 items

### Test Case 5: Single Thread (Edge Case)
```bash
./producer_consumer 1 1 5
```
- 1 producer thread
- 1 consumer thread
- Buffer size of 5 slots
- Total items: 1 × 20 = 20 items

## Key Design Features

### 1. Circular Buffer
The buffer uses a circular queue implementation with head and tail pointers that wrap around when reaching the end of the array.

### 2. Synchronization Mechanisms

**Semaphores**:
- `empty`: Tracks the number of empty slots (initialized to buffer_size)
- `full`: Tracks the number of filled slots (initialized to 0)

**Mutex**:
- `mutex`: Ensures mutual exclusion when accessing the buffer
- `stats_mutex`: Protects shared statistics counters

### 3. Priority Handling
- Items have a priority field: 0 (normal) or 1 (urgent)
- When inserting urgent items, they are placed ahead of normal priority items
- FIFO order is maintained within each priority level
- Approximately 25% of items are marked as urgent

### 4. Poison Pill Technique
After all producers finish:
1. Main thread inserts one poison pill per consumer
2. Poison pills have value -1 and urgent priority
3. When a consumer receives a poison pill, it terminates gracefully
4. Ensures all consumers exit cleanly

### 5. Metrics Tracking
- **Latency**: Time difference between enqueue and dequeue for each item
- **Throughput**: Total items consumed divided by total execution time
- Metrics are displayed after all threads complete

## Error Handling

The application handles:
- Invalid command-line arguments
- Memory allocation failures
- Thread creation failures
- Semaphore/mutex initialization failures

All errors are reported with descriptive messages and cause graceful termination.

## Thread Safety

- No race conditions: All shared data access is protected by mutexes
- No deadlocks: Consistent lock ordering and proper resource release
- No busy-waiting: Threads block on semaphores when waiting for buffer space/data
- No data corruption: Mutual exclusion ensures atomic buffer operations

## Performance Comparison

Run the following tests to compare performance metrics:

**Small Buffer Test**:
```bash
./producer_consumer 8 8 2
```

**Large Buffer Test**:
```bash
./producer_consumer 8 8 32
```

Compare the average latency and throughput values to observe how buffer size affects performance.

## Output Format

- `[Producer-X]`: Messages from producer thread X
- `[Consumer-X]`: Messages from consumer thread X
- Priority indicators: NORMAL or URGENT
- Latency displayed in seconds (microsecond precision)
- Final metrics summary at program completion

## Troubleshooting

**Issue**: Program hangs
- Check that buffer size is appropriate for the number of threads
- Ensure no deadlock by verifying proper semaphore/mutex usage

**Issue**: Compilation errors
- Verify GCC is installed: `gcc --version`
- Ensure pthread library is available: `ldconfig -p | grep pthread`

**Issue**: Segmentation fault
- Check command-line arguments are valid positive integers
- Ensure sufficient system memory for the requested configuration

## Implementation Notes

- **Language**: C with POSIX pthread library
- **Headers**: pthread.h, semaphore.h, time.h, sys/time.h
- **Thread-safe random**: Uses `rand_r()` with per-thread seeds
- **Time measurement**: Uses `gettimeofday()` for microsecond precision
- **Memory management**: All dynamically allocated memory is properly freed

## Files Included

- `producer_consumer.c`: Main source code
- `README.md`: This file
- `REPORT.md`: Design decisions and challenges documentation

## Authors

[Add your group member names here]

## Submission

- **Course**: Operating Systems
- **Deadline**: November 16, 2025
- **Platform**: iLearn

