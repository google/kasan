// SPDX-License-Identifier: GPL-2.0-only
/*
 * Compile-only tests for common patterns that should not generate false
 * positive errors when compiled with Clang's capability analysis.
 */

#include <linux/build_bug.h>
#include <linux/spinlock.h>

/*
 * Test that helper macros work as expected.
 */
static void __used test_common_helpers(void)
{
	BUILD_BUG_ON(capability_unsafe(3) != 3); /* plain expression */
	BUILD_BUG_ON(capability_unsafe((void)2; 3;) != 3); /* does not swallow semi-colon */
	BUILD_BUG_ON(capability_unsafe((void)2, 3) != 3); /* does not swallow commas */
	capability_unsafe(do { } while (0)); /* works with void statements */
}

#define TEST_SPINLOCK_COMMON(class, type, type_init, type_lock, type_unlock, type_trylock, op)	\
	struct test_##class##_data {								\
		type lock;									\
		int counter __var_guarded_by(&lock);						\
		int *pointer __ref_guarded_by(&lock);						\
	};											\
	static void __used test_##class##_init(struct test_##class##_data *d)			\
	{											\
		type_init(&d->lock);								\
		d->counter = 0;									\
	}											\
	static void __used test_##class(struct test_##class##_data *d)				\
	{											\
		unsigned long flags;								\
		d->pointer++;									\
		type_lock(&d->lock);								\
		op(d->counter);									\
		op(*d->pointer);								\
		type_unlock(&d->lock);								\
		type_lock##_irq(&d->lock);							\
		op(d->counter);									\
		op(*d->pointer);								\
		type_unlock##_irq(&d->lock);							\
		type_lock##_bh(&d->lock);							\
		op(d->counter);									\
		op(*d->pointer);								\
		type_unlock##_bh(&d->lock);							\
		type_lock##_irqsave(&d->lock, flags);						\
		op(d->counter);									\
		op(*d->pointer);								\
		type_unlock##_irqrestore(&d->lock, flags);					\
	}											\
	static void __used test_##class##_trylock(struct test_##class##_data *d)		\
	{											\
		if (type_trylock(&d->lock)) {							\
			op(d->counter);								\
			type_unlock(&d->lock);							\
		}										\
	}											\
	static void __used test_##class##_assert(struct test_##class##_data *d)			\
	{											\
		lockdep_assert_held(&d->lock);							\
		op(d->counter);									\
	}											\
	static void __used test_##class##_guard(struct test_##class##_data *d)			\
	{											\
		{ guard(class)(&d->lock);		op(d->counter); }			\
		{ guard(class##_irq)(&d->lock);		op(d->counter); }			\
		{ guard(class##_irqsave)(&d->lock);	op(d->counter); }			\
	}

#define TEST_OP_RW(x) (x)++
#define TEST_OP_RO(x) ((void)(x))

TEST_SPINLOCK_COMMON(raw_spinlock,
		     raw_spinlock_t,
		     raw_spin_lock_init,
		     raw_spin_lock,
		     raw_spin_unlock,
		     raw_spin_trylock,
		     TEST_OP_RW);
static void __used test_raw_spinlock_trylock_extra(struct test_raw_spinlock_data *d)
{
	unsigned long flags;

	if (raw_spin_trylock_irq(&d->lock)) {
		d->counter++;
		raw_spin_unlock_irq(&d->lock);
	}
	if (raw_spin_trylock_irqsave(&d->lock, flags)) {
		d->counter++;
		raw_spin_unlock_irqrestore(&d->lock, flags);
	}
	scoped_cond_guard(raw_spinlock_try, return, &d->lock) {
		d->counter++;
	}
}

TEST_SPINLOCK_COMMON(spinlock,
		     spinlock_t,
		     spin_lock_init,
		     spin_lock,
		     spin_unlock,
		     spin_trylock,
		     TEST_OP_RW);
static void __used test_spinlock_trylock_extra(struct test_spinlock_data *d)
{
	unsigned long flags;

	if (spin_trylock_irq(&d->lock)) {
		d->counter++;
		spin_unlock_irq(&d->lock);
	}
	if (spin_trylock_irqsave(&d->lock, flags)) {
		d->counter++;
		spin_unlock_irqrestore(&d->lock, flags);
	}
	scoped_cond_guard(spinlock_try, return, &d->lock) {
		d->counter++;
	}
}

TEST_SPINLOCK_COMMON(write_lock,
		     rwlock_t,
		     rwlock_init,
		     write_lock,
		     write_unlock,
		     write_trylock,
		     TEST_OP_RW);
static void __used test_write_trylock_extra(struct test_write_lock_data *d)
{
	unsigned long flags;

	if (write_trylock_irqsave(&d->lock, flags)) {
		d->counter++;
		write_unlock_irqrestore(&d->lock, flags);
	}
}

TEST_SPINLOCK_COMMON(read_lock,
		     rwlock_t,
		     rwlock_init,
		     read_lock,
		     read_unlock,
		     read_trylock,
		     TEST_OP_RO);
