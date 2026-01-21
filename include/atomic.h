#ifndef CAS_LOCK_ATOMIC_H
#define CAS_LOCK_ATOMIC_H

#include <stdint.h>

/* Platform detection */
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define CAS_LOCK_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64__)
    #define CAS_LOCK_ARM64 1
#else
    #error "Unsupported platform"
#endif

/* ============================================================================
 * ARM64 (AArch64) Implementation
 * ============================================================================ */
#if CAS_LOCK_ARM64

/* Memory order barriers for ARM64 */
#define atomic_barrier() __asm__ __volatile__("dmb ish" ::: "memory")

/* Volatile load with acquire semantics */
static inline uint32_t atomic_load_acquire(const volatile uint32_t *ptr)
{
    uint32_t value;
    __asm__ __volatile__(
        "ldar %w0, [%1]"
        : "=r"(value)
        : "r"(ptr)
        : "memory"
    );
    return value;
}

/* Volatile store with release semantics */
static inline void atomic_store_release(volatile uint32_t *ptr, uint32_t value)
{
    __asm__ __volatile__(
        "stlr %w0, [%1]"
        :
        : "r"(value), "r"(ptr)
        : "memory"
    );
}

/* Atomic load (plain) */
static inline uint32_t atomic_load(const volatile uint32_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

/* Atomic store (plain) */
static inline void atomic_store(volatile uint32_t *ptr, uint32_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

/* Atomic exchange - returns old value */
static inline uint32_t atomic_xchg(volatile uint32_t *ptr, uint32_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_ACQ_REL);
}

/* Atomic compare-and-swap - returns old value */
static inline uint32_t atomic_cmpxchg(volatile uint32_t *ptr, uint32_t expected, uint32_t desired)
{
    (void)__atomic_compare_exchange_n(ptr, &expected, desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
    return expected;
}

/* Atomic fetch-and-add - returns old value */
static inline uint32_t atomic_fetch_add(volatile uint32_t *ptr, uint32_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_ACQ_REL);
}

/* Atomic AND - returns old value */
static inline uint32_t atomic_and(volatile uint32_t *ptr, uint32_t value)
{
    return __atomic_fetch_and(ptr, value, __ATOMIC_ACQ_REL);
}

/* Atomic OR - returns old value */
static inline uint32_t atomic_or(volatile uint32_t *ptr, uint32_t value)
{
    return __atomic_fetch_or(ptr, value, __ATOMIC_ACQ_REL);
}

/* Pause hint for spin loops */
static inline void cpu_pause(void)
{
    __asm__ __volatile__("yield" ::: "memory");
}

/* Read/write barriers */
#define rmb() __asm__ __volatile__("dmb ishld" ::: "memory")
#define wmb() __asm__ __volatile__("dmb ish" ::: "memory")
#define mb() __asm__ __volatile__("dmb ish" ::: "memory")

/* ============================================================================
 * x86/x86_64 Implementation (for testing on non-ARM platforms)
 * ============================================================================ */
#elif CAS_LOCK_X86

#include <immintrin.h>

/* Memory order barriers for x86 */
#define atomic_barrier() __asm__ __volatile__("" ::: "memory")

/* Volatile load with acquire semantics */
static inline uint32_t atomic_load_acquire(const volatile uint32_t *ptr)
{
    uint32_t value = __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
    return value;
}

/* Volatile store with release semantics */
static inline void atomic_store_release(volatile uint32_t *ptr, uint32_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELEASE);
}

/* Atomic load (plain) */
static inline uint32_t atomic_load(const volatile uint32_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

/* Atomic store (plain) */
static inline void atomic_store(volatile uint32_t *ptr, uint32_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

/* Atomic exchange - returns old value */
static inline uint32_t atomic_xchg(volatile uint32_t *ptr, uint32_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_ACQ_REL);
}

/* Atomic compare-and-swap - returns old value */
static inline uint32_t atomic_cmpxchg(volatile uint32_t *ptr, uint32_t expected, uint32_t desired)
{
    (void)__atomic_compare_exchange_n(ptr, &expected, desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
    return expected;
}

/* Atomic fetch-and-add - returns old value */
static inline uint32_t atomic_fetch_add(volatile uint32_t *ptr, uint32_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_ACQ_REL);
}

/* Atomic AND - returns old value */
static inline uint32_t atomic_and(volatile uint32_t *ptr, uint32_t value)
{
    return __atomic_fetch_and(ptr, value, __ATOMIC_ACQ_REL);
}

/* Atomic OR - returns old value */
static inline uint32_t atomic_or(volatile uint32_t *ptr, uint32_t value)
{
    return __atomic_fetch_or(ptr, value, __ATOMIC_ACQ_REL);
}

/* Pause hint for spin loops */
static inline void cpu_pause(void)
{
    __asm__ __volatile__("pause" ::: "memory");
}

/* Read/write barriers - x86 has strong memory ordering */
#define rmb() __asm__ __volatile__("" ::: "memory")
#define wmb() __asm__ __volatile__("" ::: "memory")
#define mb() __asm__ __volatile__("mfence" ::: "memory")

#endif

/* ============================================================================
 * Common helper functions (platform-independent)
 * ============================================================================ */

/* Atomic fetch-and-sub - returns old value */
static inline uint32_t atomic_fetch_sub(volatile uint32_t *ptr, uint32_t value)
{
    return atomic_fetch_add(ptr, (uint32_t)(-(int32_t)value));
}

/* Atomic add - returns new value */
static inline uint32_t atomic_add(volatile uint32_t *ptr, uint32_t value)
{
    return atomic_fetch_add(ptr, value) + value;
}

/* Atomic sub - returns new value */
static inline uint32_t atomic_sub(volatile uint32_t *ptr, uint32_t value)
{
    return atomic_fetch_add(ptr, (uint32_t)(-(int32_t)value)) - value;
}

/* Atomic increment */
static inline uint32_t atomic_inc(volatile uint32_t *ptr)
{
    return atomic_add(ptr, 1);
}

/* Atomic decrement */
static inline uint32_t atomic_dec(volatile uint32_t *ptr)
{
    return atomic_sub(ptr, 1);
}

/* Atomic compare-and-swap with success indication */
static inline int atomic_cmpxchg_bool(volatile uint32_t *ptr, uint32_t expected, uint32_t desired)
{
    uint32_t old = atomic_cmpxchg(ptr, expected, desired);
    return (old == expected);
}

#endif /* CAS_LOCK_ATOMIC_H */
