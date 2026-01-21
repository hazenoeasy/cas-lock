#ifndef CAS_LOCK_MCSLOCK_H
#define CAS_LOCK_MCSLOCK_H

#include "atomic.h"
#include <stdlib.h>

/*
 * MCS Lock (Mellor-Crummey and Scott)
 * A queue-based lock that scales well on many cores
 * Each thread spins on its own cache line, reducing cache contention
 */

/* MCS queue node */
typedef struct mcs_node {
    volatile struct mcs_node *next;
    volatile uint32_t locked;
    struct mcs_node *prev;  /* For debugging */
} mcs_node_t;

/* MCS lock */
typedef struct {
    volatile mcs_node_t *tail;
} mcs_lock_t;

#define MCS_LOCK_INITIALIZER {NULL}

/* Initialize MCS lock */
static inline void mcs_init(mcs_lock_t *lock)
{
    atomic_store((volatile uint32_t *)&lock->tail, 0);
}

/* Initialize a thread's MCS node */
static inline void mcs_node_init(mcs_node_t *node)
{
    node->next = NULL;
    node->locked = 0;
    node->prev = NULL;
}

/* Acquire MCS lock */
static inline void mcs_lock(mcs_lock_t *lock, mcs_node_t *node)
{
    mcs_node_t *prev;

    /* Set our node's next pointer to NULL */
    node->next = NULL;
    node->locked = 0;

    /* Atomically swap our node as the tail, getting previous tail */
    prev = (mcs_node_t *)atomic_xchg((volatile uint32_t *)&lock->tail, (uint32_t)node);

    if (prev != NULL) {
        /* There was a previous node, add ourselves to queue */
        node->locked = 1;  /* We need to wait */
        atomic_store_release((volatile uint32_t *)&prev->next, (uint32_t)node);

        /* Spin on our own locked flag */
        while (atomic_load_acquire(&node->locked) != 0) {
            cpu_pause();
        }
    }
    /* Otherwise, lock was free, we have it */
}

/* Release MCS lock */
static inline void mcs_unlock(mcs_lock_t *lock, mcs_node_t *node)
{
    mcs_node_t *next;

    /* Check if there's a successor */
    next = (mcs_node_t *)atomic_load((volatile uint32_t *)&node->next);

    if (next == NULL) {
        /* Try to set tail back to NULL */
        mcs_node_t *old_tail = node;
        if (atomic_cmpxchg_bool((volatile uint32_t *)&lock->tail,
                                (uint32_t)old_tail, 0)) {
            /* Successfully unset tail, no one is waiting */
            return;
        }

        /* Someone added themselves to queue, wait for them to set next */
        while ((next = (mcs_node_t *)atomic_load((volatile uint32_t *)&node->next)) == NULL) {
            cpu_pause();
        }
    }

    /* Pass lock to successor */
    atomic_store_release(&next->locked, 0);
}

/*
 * CLH Lock (Craig, Landin, and Hagersten)
 * Similar to MCS but with a different spinning strategy
 */

typedef struct clh_node {
    volatile struct clh_node *prev;
    volatile uint32_t locked;
} clh_node_t;

typedef struct {
    volatile clh_node_t *tail;
} clh_lock_t;

#define CLH_LOCK_INITIALIZER {NULL}
#define CLH_NODE_INITIALIZER {NULL, 0}

static inline void clh_init(clh_lock_t *lock)
{
    clh_node_t *dummy = (clh_node_t *)malloc(sizeof(clh_node_t));
    dummy->prev = NULL;
    dummy->locked = 0;
    atomic_store((volatile uint32_t *)&lock->tail, (uint32_t)dummy);
}

static inline void clh_node_init(clh_node_t *node)
{
    node->locked = 0;
    node->prev = NULL;
}

static inline void clh_lock(clh_lock_t *lock, clh_node_t *node)
{
    clh_node_t *prev;

    node->locked = 1;
    prev = (clh_node_t *)atomic_xchg((volatile uint32_t *)&lock->tail, (uint32_t)node);
    node->prev = prev;

    /* Spin on predecessor's locked flag */
    while (atomic_load_acquire(&prev->locked) != 0) {
        cpu_pause();
    }
}

static inline void clh_unlock(clh_lock_t *lock, clh_node_t *node)
{
    clh_node_t *prev = node->prev;

    /* Signal that we're done */
    atomic_store_release(&node->locked, 0);

    /* Must keep node for GC - in practice, use a persistent node per thread */
}

#endif /* CAS_LOCK_MCSLOCK_H */
