#pragma once
#ifndef KERNEL_SPINLOCK_H
#define KERNEL_SPINLOCK_H

#include <stdatomic.h>

#include <kernel/isr.h>

typedef struct {
	volatile uint32_t locked;
} spinlock_t;

#define SPINLOCK_INITIALIZER {.locked = 0}

static inline void spinlock_init(spinlock_t *lock) {
	lock->locked = 0;
}

static inline void spin_acquire(spinlock_t *lock) {
	while (lock->locked || __atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
		// Busy-wait (spin)
	}
}

static inline void spin_acquire_irq(spinlock_t *lock) {
	irq_disable();
	spin_acquire(lock);
}

static inline uint64_t spin_acquire_irqsave(spinlock_t *lock) {
	uint64_t flags = irq_save();
	spin_acquire_irq(lock);
	return flags;
}

static inline void spin_release(spinlock_t *lock) {
	atomic_flag_clear(&lock->locked);
}

static inline void spin_release_irq(spinlock_t *lock) {
	spin_release(lock);
	irq_enable();
}

static inline void spin_release_irqrestore(spinlock_t *lock, uint64_t flags) {
	spin_release(lock);
	irq_restore(flags);
}

typedef struct {
	spinlock_t reader_lock;
	spinlock_t writer_lock;
	uint32_t readers;
} rwlock_t;

#define RWLOCK_INITIALIZER {             \
	.readers = 0,                        \
	.reader_lock = SPINLOCK_INITIALIZER, \
	.writer_lock = SPINLOCK_INITIALIZER, \
}

static inline void rwlock_init(rwlock_t *lock) {
	lock->readers = 0;
	spinlock_init(&lock->reader_lock);
	spinlock_init(&lock->writer_lock);
}

static inline void read_acquire(rwlock_t *lock) {
	spin_acquire(&lock->reader_lock);
	if (++lock->readers == 1)
		spin_acquire(&lock->writer_lock);
	spin_release(&lock->reader_lock);
}

static inline void read_acquire_irq(rwlock_t *lock) {
	irq_disable();
	read_acquire(lock);
}

static inline uint64_t read_acquire_irqsave(rwlock_t *lock) {
	uint64_t flags = irq_save();
	read_acquire_irq(lock);
	return flags;
}

static inline void read_release(rwlock_t *lock) {
	if (__atomic_sub_fetch(&lock->readers, 1, __ATOMIC_SEQ_CST) == 0)
		spin_release(&lock->writer_lock);
}

static inline void read_release_irq(rwlock_t *lock) {
	read_release(lock);
	irq_enable();
}

static inline void read_release_irqrestore(rwlock_t *lock, uint64_t flags) {
	read_release(lock);
	irq_restore(flags);
}

static inline void write_acquire(rwlock_t *lock) {
	spin_acquire(&lock->writer_lock);
}

static inline void write_acquire_irq(rwlock_t *lock) {
	irq_disable();
	write_acquire(lock);
}

static inline uint64_t write_acquire_irqsave(rwlock_t *lock) {
	uint64_t flags = irq_save();
	write_acquire_irq(lock);
	return flags;
}

static inline void write_release(rwlock_t *lock) {
	spin_release(&lock->writer_lock);
}

static inline void write_release_irq(rwlock_t *lock) {
	write_release(lock);
	irq_enable();
}

static inline void write_release_irqrestore(rwlock_t *lock, uint64_t flags) {
	write_release(lock);
	irq_restore(flags);
}

#endif
