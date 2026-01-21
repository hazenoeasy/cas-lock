#ifndef CAS_LOCK_SPINLOCK_H
#define CAS_LOCK_SPINLOCK_H

#include "atomic.h"

/*
 * Simple Test-And-Set (TAS) Spinlock
 * Uses atomic exchange to acquire the lock
 */
typedef struct {
    volatile uint32_t locked;
} spinlock_t;

#define SPINLOCK_INITIALIZER {0}

/* Initialize spinlock */
static inline void spin_init(spinlock_t *lock)
{
    atomic_store(&lock->locked, 0);
}

/* Try to acquire lock - returns 1 on success, 0 on failure */
static inline int spin_trylock(spinlock_t *lock)
{
    return atomic_xchg(&lock->locked, 1) == 0;
}

/* Acquire lock with spinning */
static inline void spin_lock(spinlock_t *lock)
{
    while (atomic_xchg(&lock->locked, 1) != 0) {
        /* Reduce bus contention */
        cpu_pause();
    }
}

/* Release lock */
static inline void spin_unlock(spinlock_t *lock)
{
    atomic_store_release(&lock->locked, 0);
}

/*
 * Test-And-Test-And-Set (TATAS) Spinlock
 * First checks if lock is available before attempting atomic exchange
 */
typedef struct {
    volatile uint32_t locked;
} tatas_lock_t;

#define TATAS_LOCK_INITIALIZER {0}

static inline void tatas_init(tatas_lock_t *lock)
{
    atomic_store(&lock->locked, 0);
}

static inline int tatas_trylock(tatas_lock_t *lock)
{
    return atomic_xchg(&lock->locked, 1) == 0;
}

static inline void tatas_lock(tatas_lock_t *lock)
{
    while (1) {
        /* First check without atomic operation */
        if (atomic_load(&lock->locked) == 0) {
            /* Then try to acquire atomically */
            if (atomic_xchg(&lock->locked, 1) == 0) {
                return;
            }
        }
        cpu_pause();
    }
}

static inline void tatas_unlock(tatas_lock_t *lock)
{
    atomic_store_release(&lock->locked, 0);
}

#endif /* CAS_LOCK_SPINLOCK_H */
