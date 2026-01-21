#ifndef CAS_LOCK_TICKETLOCK_H
#define CAS_LOCK_TICKETLOCK_H

#include "atomic.h"

/*
 * Ticket Lock - Fair spinlock
 * Ensures FIFO ordering of lock acquisition
 * Similar to a bakery/deli ticket system
 */
typedef struct {
    volatile uint32_t next_ticket;
    volatile uint32_t serving;
} ticketlock_t;

#define TICKETLOCK_INITIALIZER {0, 0}

/* Initialize ticket lock */
static inline void ticket_init(ticketlock_t *lock)
{
    atomic_store(&lock->next_ticket, 0);
    atomic_store(&lock->serving, 0);
}

/* Acquire lock */
static inline void ticket_lock(ticketlock_t *lock)
{
    /* Get my ticket */
    uint32_t my_ticket = atomic_fetch_add(&lock->next_ticket, 1);

    /* Wait until my ticket is served */
    while (atomic_load_acquire(&lock->serving) != my_ticket) {
        cpu_pause();
    }
}

/* Try to acquire lock - returns 1 on success, 0 on failure */
static inline int ticket_trylock(ticketlock_t *lock)
{
    uint32_t next_ticket = atomic_load(&lock->next_ticket);
    uint32_t serving = atomic_load(&lock->serving);

    /* Lock is already contended */
    if (next_ticket != serving) {
        return 0;
    }

    /* Try to claim the ticket */
    if (atomic_cmpxchg_bool(&lock->next_ticket, next_ticket, next_ticket + 1)) {
        /* Verify no one else claimed it */
        return (atomic_load_acquire(&lock->serving) == next_ticket);
    }

    return 0;
}

/* Release lock */
static inline void ticket_unlock(ticketlock_t *lock)
{
    uint32_t next = atomic_load(&lock->serving) + 1;
    atomic_store_release(&lock->serving, next);
}

/*
 * Anderson Lock - Array-based queue lock
 * Reduces contention by having each thread spin on a different cache line
 */
#define ANDERSON_LOCK_MAX_THREADS 64

typedef struct {
    volatile uint32_t next_slot;
    volatile uint32_t serving_slot;
    volatile uint32_t flags[ANDERSON_LOCK_MAX_THREADS];
    uint32_t num_slots;
} anderson_lock_t;

#define ANDERSON_LOCK_INITIALIZER(slots) {0, 0, {[0 ... (slots)-1] = 1}, slots}

/* Initialize Anderson lock */
static inline void anderson_init(anderson_lock_t *lock, uint32_t num_slots)
{
    int i;
    if (num_slots > ANDERSON_LOCK_MAX_THREADS) {
        num_slots = ANDERSON_LOCK_MAX_THREADS;
    }
    lock->num_slots = num_slots;
    atomic_store(&lock->next_slot, 0);
    atomic_store(&lock->serving_slot, 0);

    /* First slot is available, rest are not */
    for (i = 0; i < num_slots; i++) {
        atomic_store(&lock->flags[i], (i == 0) ? 1 : 0);
    }
}

/* Acquire Anderson lock */
static inline void anderson_lock(anderson_lock_t *lock)
{
    /* Get my slot */
    uint32_t my_slot = atomic_fetch_add(&lock->next_slot, 1) % lock->num_slots;

    /* Wait for my slot's flag to be set */
    while (atomic_load_acquire(&lock->flags[my_slot]) == 0) {
        cpu_pause();
    }

    /* Clear my flag for the next thread */
    atomic_store_release(&lock->flags[my_slot], 0);
}

/* Release Anderson lock */
static inline void anderson_unlock(anderson_lock_t *lock)
{
    /* Move to next slot and set its flag */
    uint32_t next_slot = atomic_load(&lock->serving_slot) + 1;
    atomic_store(&lock->serving_slot, next_slot % lock->num_slots);
    atomic_store_release(&lock->flags[next_slot % lock->num_slots], 1);
}

#endif /* CAS_LOCK_TICKETLOCK_H */
