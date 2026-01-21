#ifndef CAS_LOCK_RWLOCK_H
#define CAS_LOCK_RWLOCK_H

#include "atomic.h"

/*
 * Simple Reader-Writer Lock
 * Multiple readers can hold the lock simultaneously
 * Writers have exclusive access
 * Writer-preferring implementation
 */
typedef struct {
    volatile uint32_t readers;
    volatile uint32_t writer;
} rwlock_t;

#define RWLOCK_INITIALIZER {0, 0}

/* Initialize rwlock */
static inline void rw_init(rwlock_t *lock)
{
    atomic_store(&lock->readers, 0);
    atomic_store(&lock->writer, 0);
}

/* Acquire read lock */
static inline void rw_read_lock(rwlock_t *lock)
{
    /* Wait while there's an active or waiting writer */
    while (1) {
        /* Check if no writer */
        if (atomic_load(&lock->writer) == 0) {
            /* Try to increment reader count */
            uint32_t old_readers = atomic_load(&lock->readers);
            if (atomic_cmpxchg_bool(&lock->readers, old_readers, old_readers + 1)) {
                /* Verify no writer acquired while we were incrementing */
                if (atomic_load(&lock->writer) == 0) {
                    return;
                }
                /* Writer acquired, back off */
                atomic_dec(&lock->readers);
            }
        }
        cpu_pause();
    }
}

/* Try to acquire read lock - returns 1 on success */
static inline int rw_read_trylock(rwlock_t *lock)
{
    if (atomic_load(&lock->writer) != 0) {
        return 0;
    }
    uint32_t old_readers = atomic_load(&lock->readers);
    if (atomic_cmpxchg_bool(&lock->readers, old_readers, old_readers + 1)) {
        if (atomic_load(&lock->writer) == 0) {
            return 1;
        }
        atomic_dec(&lock->readers);
    }
    return 0;
}

/* Release read lock */
static inline void rw_read_unlock(rwlock_t *lock)
{
    atomic_dec(&lock->readers);
}

/* Acquire write lock */
static inline void rw_write_lock(rwlock_t *lock)
{
    /* Announce we want to write */
    while (atomic_xchg(&lock->writer, 1) != 0) {
        cpu_pause();
    }

    /* Wait for all readers to finish */
    while (atomic_load(&lock->readers) != 0) {
        cpu_pause();
    }
}

/* Try to acquire write lock - returns 1 on success */
static inline int rw_write_trylock(rwlock_t *lock)
{
    if (atomic_xchg(&lock->writer, 1) != 0) {
        return 0;
    }
    if (atomic_load(&lock->readers) != 0) {
        atomic_store(&lock->writer, 0);
        return 0;
    }
    return 1;
}

/* Release write lock */
static inline void rw_write_unlock(rwlock_t *lock)
{
    atomic_store_release(&lock->writer, 0);
}

/*
 * Phase Fairs Reader-Writer Lock
 * Alternates between reader and writer phases
 */
typedef struct {
    volatile uint32_t readers;
    volatile uint32_t writers;
    volatile uint32_t writer_active;
    volatile uint32_t read_phase;
} rwlock_phase_t;

#define RWLOCK_PHASE_INITIALIZER {0, 0, 0, 0}

static inline void rw_phase_init(rwlock_phase_t *lock)
{
    atomic_store(&lock->readers, 0);
    atomic_store(&lock->writers, 0);
    atomic_store(&lock->writer_active, 0);
    atomic_store(&lock->read_phase, 0);
}

static inline void rw_phase_read_lock(rwlock_phase_t *lock)
{
    while (1) {
        /* Wait if writer active or write phase */
        if (atomic_load(&lock->writer_active) == 0 &&
            atomic_load(&lock->read_phase) == 1) {

            uint32_t old_readers = atomic_load(&lock->readers);
            if (atomic_cmpxchg_bool(&lock->readers, old_readers, old_readers + 1)) {
                if (atomic_load(&lock->writer_active) == 0) {
                    return;
                }
                atomic_dec(&lock->readers);
            }
        }
        cpu_pause();
    }
}

static inline void rw_phase_read_unlock(rwlock_phase_t *lock)
{
    atomic_dec(&lock->readers);
}

static inline void rw_phase_write_lock(rwlock_phase_t *lock)
{
    /* Enter write phase */
    atomic_inc(&lock->writers);
    atomic_store(&lock->read_phase, 0);

    /* Wait for readers to finish */
    while (atomic_load(&lock->readers) != 0) {
        cpu_pause();
    }

    /* Try to become active writer */
    while (atomic_xchg(&lock->writer_active, 1) != 0) {
        cpu_pause();
    }

    atomic_dec(&lock->writers);
}

static inline void rw_phase_write_unlock(rwlock_phase_t *lock)
{
    atomic_store_release(&lock->writer_active, 0);
    atomic_store(&lock->read_phase, 1);
}

#endif /* CAS_LOCK_RWLOCK_H */
