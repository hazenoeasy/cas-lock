# CAS Lock Library

一套基于 ARM 原子指令的用户态锁库，使用 C 语言实现，专为 ARM64 (AArch64) 架构优化，同时支持 x86_64 平台。

## 特性

- **纯 C 实现** - 无外部依赖，易于集成
- **多平台支持** - ARM64 (AArch64) 和 x86_64
- **多种锁类型** - 自旋锁、Ticket Lock、读写锁等
- **高性能** - 使用 ARM64 LDXR/STXR 指令和 GCC/Clang 内置原子操作
- **内存序支持** - 正确处理 acquire/release 语义

## 实现的锁类型

| 锁类型 | 描述 | 适用场景 |
|--------|------|----------|
| Spinlock (TAS) | Test-And-Set 自旋锁 | 短临界区 |
| TATAS Lock | Test-And-Test-And-Set，减少总线争用 | 中等并发 |
| Ticket Lock | 公平的排队锁 | 需要公平性保证 |
| Anderson Lock | 基于数组的队列锁 | 固定线程数场景 |
| RWLock | 读写锁，支持多读者 | 读多写少场景 |
| MCS Lock | 基于链表的可扩展锁 | 高并发场景（开发中） |

## 编译

### 要求

- GCC 或 Clang 编译器
- 支持 ARM64 或 x86_64 架构
- pthread 库

### 编译命令

```bash
make          # 编译所有目标
make test     # 编译并运行正确性测试
make bench    # 编译并运行性能测试
make clean    # 清理构建产物
```

## 使用示例

```c
#include "atomic.h"
#include "spinlock.h"

spinlock_t lock = SPINLOCK_INITIALIZER;
volatile uint32_t counter = 0;

// 线程函数
void* thread_func(void* arg) {
    for (int i = 0; i < 10000; i++) {
        spin_lock(&lock);
        counter++;
        spin_unlock(&lock);
    }
    return NULL;
}
```

## API 参考

### 原子操作 (atomic.h)

```c
// 基础操作
uint32_t atomic_load(const volatile uint32_t *ptr);
void atomic_store(volatile uint32_t *ptr, uint32_t value);

// 复合操作
uint32_t atomic_xchg(volatile uint32_t *ptr, uint32_t value);
uint32_t atomic_cmpxchg(volatile uint32_t *ptr, uint32_t expected, uint32_t desired);
uint32_t atomic_fetch_add(volatile uint32_t *ptr, uint32_t value);
uint32_t atomic_and(volatile uint32_t *ptr, uint32_t value);
uint32_t atomic_or(volatile uint32_t *ptr, uint32_t value);

// 自旋提示
void cpu_pause(void);
```

### 自旋锁 (spinlock.h)

```c
spinlock_t lock = SPINLOCK_INITIALIZER;

void spin_init(spinlock_t *lock);
int spin_trylock(spinlock_t *lock);  // 返回 1 成功，0 失败
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
```

### Ticket Lock (ticketlock.h)

```c
ticketlock_t lock = TICKETLOCK_INITIALIZER;

void ticket_init(ticketlock_t *lock);
int ticket_trylock(ticketlock_t *lock);
void ticket_lock(ticketlock_t *lock);
void ticket_unlock(ticketlock_t *lock);
```

### 读写锁 (rwlock.h)

```c
rwlock_t lock = RWLOCK_INITIALIZER;

void rw_init(rwlock_t *lock);
void rw_read_lock(rwlock_t *lock);
int rw_read_trylock(rwlock_t *lock);
void rw_read_unlock(rwlock_t *lock);
void rw_write_lock(rwlock_t *lock);
int rw_write_trylock(rwlock_t *lock);
void rw_write_unlock(rwlock_t *lock);
```

## 性能基准 (Apple Silicon M1/M2)

```
Lock Type       |  Threads |    Ops/sec
----------------|----------|-------------
TATAS Lock      |        1 |   221M
Ticket Lock     |        1 |   170M
RWLock (write)  |        1 |   214M
TATAS Lock      |        8 |    9.9M
Ticket Lock     |        8 |    3.3M
```

## ARM64 原子指令

本库使用 ARM64 的 Load-Exclusive/Store-Exclusive 指令对实现无锁算法：

- `LDXR` - 加载独占
- `STXR` - 存储独占
- `LDAR` - 加载获取
- `STLR` - 存储释放

## 许可证

MIT License

## 作者

CAS Lock Library Contributors
