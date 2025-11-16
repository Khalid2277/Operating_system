# Producer-Consumer Simplified Implementation

## Overview

This is a **simplified** implementation of the Producer-Consumer problem that **strictly follows** the patterns shown in the Operating Systems textbook slides (Chapters 3 & 7).

## Key Differences from Original Version

### What's the Same:
- ✅ Core synchronization (semaphores + mutex)
- ✅ Circular bounded buffer
- ✅ Multiple producers/consumers
- ✅ Poison pill termination
- ✅ Priority handling (bonus)
- ✅ Performance metrics (bonus)

### What's Simplified:
- **Code Structure**: Follows slide pseudocode more closely
- **Variable Names**: Uses `in`/`out` from Chapter 3, page 29
- **Semaphore Names**: Uses `mutex`, `empty`, `full` from Chapter 7, pages 4-6
- **Comments**: References specific slide chapters/pages
- **Priority Logic**: Simpler shifting algorithm
- **No Separate Stats File**: All metrics in main file

## Compilation

```bash
gcc -o producer_consumer_simplified producer_consumer_simplified.c -pthread
```

## Usage

```bash
./producer_consumer_simplified <num_producers> <num_consumers> <buffer_size>
```

### Example:
```bash
./producer_consumer_simplified 3 2 10
```

## Code Mapping to Slides

### Chapter 3, Page 29 - Bounded Buffer Structure
```c
// Slide shows:
item buffer[BUFFER_SIZE];
int in = 0;
int out = 0;

// Our implementation:
item *buffer;  // dynamically allocated
int in = 0;    // tail index
int out = 0;   // head index
```

### Chapter 7, Pages 4-6 - Semaphore Solution
```c
// Slide shows:
sem_t mutex;  // initialized to 1
sem_t empty;  // initialized to n
sem_t full;   // initialized to 0

// Producer (page 5):
wait(empty);
wait(mutex);
// add item to buffer
signal(mutex);
signal(full);

// Consumer (page 6):
wait(full);
wait(mutex);
// remove item from buffer
signal(mutex);
signal(empty);
```

Our `insert_item()` and `remove_item()` functions follow this **exact pattern**.

## Features

### Core Features (from slides)
1. **Bounded Buffer**: Circular array with `in` and `out` indices
2. **Semaphores**: `mutex`, `empty`, `full` for synchronization
3. **Multiple Threads**: Configurable producers and consumers
4. **No Busy-Waiting**: Threads block on semaphores

### Bonus Features
1. **Priority Handling (+5%)**: 
   - Each item has priority (0=normal, 1=urgent)
   - Urgent items processed before normal items
   - Simple shifting algorithm (not in slides)
   - 25% of items are urgent

2. **Performance Metrics (+5%)**:
   - Tracks latency (time from produce to consume)
   - Calculates throughput (items/second)
   - Uses `gettimeofday()` for timestamps

## Test Cases

### Basic Test:
```bash
./producer_consumer_simplified 2 2 5
```

### High Contention Test:
```bash
./producer_consumer_simplified 5 5 2
```

### Low Contention Test:
```bash
./producer_consumer_simplified 5 5 32
```

## Output Format

```
Configuration: 2 producers, 2 consumers, buffer size = 5
Each producer generates 20 items

Creating 2 producer thread(s)...
Creating 2 consumer thread(s)...

[P1] Produced: 645 (Priority: URGENT)
[C1] Consumed: 645 (Priority: URGENT, Latency: 0.000016 sec)
...
All producers finished.
Inserting 2 poison pill(s)...
All consumers finished.

========== Performance Metrics ==========
Total items produced: 40
Total items consumed: 40
Total execution time: 0.001234 seconds
Average latency: 0.000018 seconds
Throughput: 32414.91 items/second
=========================================
```

## Why This Version?

This simplified version:
- **Matches slide terminology** exactly (in/out, mutex/empty/full)
- **References specific chapters** in comments
- **Uses slide pseudocode structure** for producer/consumer
- **Simpler to understand** and grade
- **Still includes all bonus features**

Use this version if you want to demonstrate that your implementation **directly follows the textbook slides**.

## Platform Requirements

- **Linux ONLY** (POSIX semaphores)
- GCC with pthread support
- Not compatible with macOS (sem_init deprecated)

## File Comparison

| Feature | `producer_consumer.c` | `producer_consumer_simplified.c` |
|---------|----------------------|----------------------------------|
| Core Logic | ✅ Correct | ✅ Correct |
| Slide Terminology | ❌ Different names | ✅ Exact match |
| Code Complexity | Higher | Lower |
| Comments | Implementation-focused | Slide-reference focused |
| Priority Logic | Complex shifting | Simple shifting |
| Recommended For | Advanced implementation | Grading/demonstration |

## Author Notes

This version was created to:
1. Match the textbook slides as closely as possible
2. Simplify grading by using familiar terminology
3. Demonstrate understanding of core concepts from Chapters 3, 6, and 7
4. Keep bonus features while reducing complexity

Both versions are fully functional and correct.

