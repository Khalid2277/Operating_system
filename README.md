# Producer-Consumer Implementation

## Overview

This implementation solves the classic **Producer-Consumer problem** (bounded-buffer problem) using **POSIX threads, semaphores, and mutexes**. The solution follows the patterns from the Operating Systems textbook (Chapters 3, 6, and 7) and includes two bonus features: **priority handling** and **performance metrics**.

## Features

### Core Implementation
- **Bounded Buffer**: Circular array with `in` and `out` indices
- **Semaphore Synchronization**: `mutex`, `empty`, `full` 
- **Multiple Threads**: Configurable producers and consumers
- **No Busy-Waiting**: Threads block efficiently on semaphores
- **Poison Pill Termination**: Clean shutdown mechanism

### Bonus Features
- **Priority Handling (+5%)**: Urgent items consumed before normal items
- **Performance Metrics (+5%)**: Latency and throughput tracking

## Compilation

```bash
gcc -o producer_consumer producer_consumer.c -pthread
```

## Usage

```bash
./producer_consumer <num_producers> <num_consumers> <buffer_size>
```

### Example:
```bash
./producer_consumer 3 2 10
```

**Expected Output:** All items produced will be consumed exactly once, with urgent items processed before normal items.


## Test Cases

### Basic Test:
```bash
./producer_consumer 2 2 5
```
**Expected:** 40 items produced, 40 consumed

### High Contention Test (Small Buffer):
```bash
./producer_consumer 8 8 2
```
**Expected:** Higher latency due to frequent blocking

### Low Contention Test (Large Buffer):
```bash
./producer_consumer 8 8 32
```
**Expected:** Lower latency, higher throughput

### Recommended Test (Project Specs):
```bash
./producer_consumer 3 2 10
```
**Expected:** 60 items produced, 60 consumed, priority ordering visible

## Sample Output

```
Configuration: 3 producers, 2 consumers, buffer size = 10
Each producer generates 20 items

Creating 3 producer thread(s)...
Creating 2 consumer thread(s)...

[P1] Produced: 507 (Priority: URGENT)
[P2] Produced: 288 (Priority: NORMAL)
[P2] Produced: 607 (Priority: URGENT)
[C2] Consumed: 507 (Priority: URGENT, Latency: 0.000392 sec)
[C2] Consumed: 607 (Priority: URGENT, Latency: 0.000250 sec)
[C2] Consumed: 288 (Priority: NORMAL, Latency: 0.000284 sec)
...

All producers finished.
Inserting 2 poison pill(s)...
[C1] Received poison pill. Terminating.
[C2] Received poison pill. Terminating.
All consumers finished.

========== Performance Metrics ==========
Total items produced: 60
Total items consumed: 60
Total execution time: 0.002557 seconds
Average latency: 0.000162 seconds
Throughput: 23465.00 items/second
=========================================

Program completed successfully.
```

**Key Observations:**
- URGENT items (507, 607) consumed before NORMAL items (288)
- All 60 items produced are consumed (no item loss)
- Poison pills consumed last (after all real items)
- Clean termination with performance metrics

## Platform Requirements

- **Linux** (POSIX semaphores)
- GCC with pthread support


## Implementation Highlights

### Correctness Guarantees
- **No Race Conditions**: All critical sections protected by mutex
- **No Deadlocks**: Proper semaphore ordering
- **No Busy-Waiting**: Threads block on semaphores
- **100% Reliability**: All produced items consumed exactly once
- **Thread-Safe Random**: Uses `rand_r()` with per-thread seeds

### Synchronization Pattern
```c
Producer:                    Consumer:
  sem_wait(&empty)            sem_wait(&full)
  sem_wait(&mutex)            sem_wait(&mutex)
  // produce                  // consume (with priority)
  sem_post(&mutex)            sem_post(&mutex)
  sem_post(&full)             sem_post(&empty)
```

## Authors

- Khalid Alfahim (b00100122)
- Ahmad Mustafawi (b00094873)

**Course:** CMP 310 - Operating Systems  
**Institution:** American University of Sharjah  
**Date:** November 16, 2025

