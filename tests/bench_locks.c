/*
 * Performance Benchmarks for Atomic Locks
 * Compares performance of different lock implementations
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "../include/atomic.h"
#include "../include/spinlock.h"
#include "../include/ticketlock.h"
#include "../include/rwlock.h"
#include "../include/mcslock.h"

/* Benchmark configuration */
#define BENCH_ITERATIONS 10000000
#define NUM_THREADS_LIST {1, 2, 4, 8}
#define NUM_THREAD_CONFIGS 4

/* Time measurement */
static uint64_t nanos(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* Benchmark result */
typedef struct {
    const char *name;
    uint64_t ns;
    double ops_per_sec;
} bench_result_t;

/* Shared counter */
static volatile uint32_t counter;

/* ==================== Spinlock Benchmark ==================== */

static spinlock_t g_spin_lock;

static void* spinlock_bench_thread(void *arg)
{
    uint64_t iterations = *(uint64_t*)arg;
    uint64_t i;

    for (i = 0; i < iterations; i++) {
        spin_lock(&g_spin_lock);
        counter++;
        spin_unlock(&g_spin_lock);
    }
    return NULL;
}

static bench_result_t bench_spinlock(int num_threads)
{
    pthread_t threads[num_threads];
    uint64_t iterations = BENCH_ITERATIONS / num_threads;
    uint64_t start, end;
    int i;

    counter = 0;
    spin_init(&g_spin_lock);

    start = nanos();
    for (i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, spinlock_bench_thread, &iterations);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    end = nanos();

    bench_result_t result = {
        .name = "Spinlock",
        .ns = end - start,
        .ops_per_sec = (double)BENCH_ITERATIONS * 1e9 / (end - start)
    };
    return result;
}

/* ==================== TATAS Lock Benchmark ==================== */

static tatas_lock_t g_tatas_lock;

static void* tatas_bench_thread(void *arg)
{
    uint64_t iterations = *(uint64_t*)arg;
    uint64_t i;

    for (i = 0; i < iterations; i++) {
        tatas_lock(&g_tatas_lock);
        counter++;
        tatas_unlock(&g_tatas_lock);
    }
    return NULL;
}

static bench_result_t bench_tatas_lock(int num_threads)
{
    pthread_t threads[num_threads];
    uint64_t iterations = BENCH_ITERATIONS / num_threads;
    uint64_t start, end;
    int i;

    counter = 0;
    tatas_init(&g_tatas_lock);

    start = nanos();
    for (i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, tatas_bench_thread, &iterations);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    end = nanos();

    bench_result_t result = {
        .name = "TATAS Lock",
        .ns = end - start,
        .ops_per_sec = (double)BENCH_ITERATIONS * 1e9 / (end - start)
    };
    return result;
}

/* ==================== Ticket Lock Benchmark ==================== */

static ticketlock_t g_ticket_lock;

static void* ticketlock_bench_thread(void *arg)
{
    uint64_t iterations = *(uint64_t*)arg;
    uint64_t i;

    for (i = 0; i < iterations; i++) {
        ticket_lock(&g_ticket_lock);
        counter++;
        ticket_unlock(&g_ticket_lock);
    }
    return NULL;
}

static bench_result_t bench_ticketlock(int num_threads)
{
    pthread_t threads[num_threads];
    uint64_t iterations = BENCH_ITERATIONS / num_threads;
    uint64_t start, end;
    int i;

    counter = 0;
    ticket_init(&g_ticket_lock);

    start = nanos();
    for (i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, ticketlock_bench_thread, &iterations);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    end = nanos();

    bench_result_t result = {
        .name = "Ticket Lock",
        .ns = end - start,
        .ops_per_sec = (double)BENCH_ITERATIONS * 1e9 / (end - start)
    };
    return result;
}

/* ==================== MCS Lock Benchmark ==================== */

static mcs_lock_t g_mcs_lock;
static __thread mcs_node_t mcs_local_node;

static void* mcslock_bench_thread(void *arg)
{
    uint64_t iterations = *(uint64_t*)arg;
    uint64_t i;

    mcs_node_init(&mcs_local_node);

    for (i = 0; i < iterations; i++) {
        mcs_lock(&g_mcs_lock, &mcs_local_node);
        counter++;
        mcs_unlock(&g_mcs_lock, &mcs_local_node);
    }
    return NULL;
}

static bench_result_t bench_mcslock(int num_threads)
{
    pthread_t threads[num_threads];
    uint64_t iterations = BENCH_ITERATIONS / num_threads;
    uint64_t start, end;
    int i;

    counter = 0;
    mcs_init(&g_mcs_lock);

    start = nanos();
    for (i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, mcslock_bench_thread, &iterations);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    end = nanos();

    bench_result_t result = {
        .name = "MCS Lock",
        .ns = end - start,
        .ops_per_sec = (double)BENCH_ITERATIONS * 1e9 / (end - start)
    };
    return result;
}

/* ==================== RWLock Benchmark ==================== */

static rwlock_t rw_lock;

static void* rwlock_bench_thread(void *arg)
{
    uint64_t iterations = *(uint64_t*)arg;
    uint64_t i;

    /* Benchmark as write lock (exclusive) */
    for (i = 0; i < iterations; i++) {
        rw_write_lock(&rw_lock);
        counter++;
        rw_write_unlock(&rw_lock);
    }
    return NULL;
}

static bench_result_t bench_rwlock(int num_threads)
{
    pthread_t threads[num_threads];
    uint64_t iterations = BENCH_ITERATIONS / num_threads;
    uint64_t start, end;
    int i;

    counter = 0;
    rw_init(&rw_lock);

    start = nanos();
    for (i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, rwlock_bench_thread, &iterations);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    end = nanos();

    bench_result_t result = {
        .name = "RWLock (write)",
        .ns = end - start,
        .ops_per_sec = (double)BENCH_ITERATIONS * 1e9 / (end - start)
    };
    return result;
}

/* ==================== Main Benchmark Runner ==================== */

typedef bench_result_t (*bench_fn)(int);

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    int thread_counts[] = {1, 2, 4, 8};
    int num_configs = 4;
    int i, j;
    bench_fn benches[] = {
        bench_spinlock,
        bench_tatas_lock,
        bench_ticketlock,
        /* MCS lock temporarily disabled - needs 64-bit atomic pointer support */
        /* bench_mcslock, */
        bench_rwlock,
    };
    int num_benches = sizeof(benches) / sizeof(benches[0]);

    printf("==========================================================\n");
    printf("CAS Lock Library - Performance Benchmarks\n");
    printf("==========================================================\n\n");

    printf("Total operations: %d per benchmark\n\n", BENCH_ITERATIONS);

    printf("%-15s | %8s | %12s | %12s\n", "Lock Type", "Threads", "Time (ms)", "Ops/sec");
    printf("----------------------------------------------------------\n");

    for (j = 0; j < num_benches; j++) {
        for (i = 0; i < num_configs; i++) {
            bench_result_t result = benches[j](thread_counts[i]);
            printf("%-15s | %8d | %12.2f | %12.0f\n",
                   result.name,
                   thread_counts[i],
                   result.ns / 1000000.0,
                   result.ops_per_sec);
        }
        printf("----------------------------------------------------------\n");
    }

    printf("\n==========================================================\n");
    printf("Benchmark Complete\n");
    printf("==========================================================\n");

    return 0;
}
