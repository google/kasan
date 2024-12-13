/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Macros and attributes for compiler-based static capability analysis.
 */

#ifndef _LINUX_COMPILER_CAPABILITY_ANALYSIS_H
#define _LINUX_COMPILER_CAPABILITY_ANALYSIS_H

#if defined(WARN_CAPABILITY_ANALYSIS)

/*
 * Tells the compiler to not do any capability analysis. Prefer
 * capability_unsafe(..) where possible.
 */
# define __no_capability_analysis	__attribute__((no_thread_safety_analysis))

/*
 * The below attributes are used to define new capability types.
 */
# define __cap_type(name)			__attribute__((capability(#name)))
# define __acquires_cap(var)			__attribute__((acquire_capability(var)))
# define __acquires_shared_cap(var)		__attribute__((acquire_shared_capability(var)))
# define __try_acquires_cap(ret, var)		__attribute__((try_acquire_capability(ret, var)))
# define __try_acquires_shared_cap(ret, var)	__attribute__((try_acquire_shared_capability(ret, var)))
# define __releases_cap(var)			__attribute__((release_capability(var)))
# define __releases_shared_cap(var)		__attribute__((release_shared_capability(var)))
# define __asserts_cap(var)			__attribute__((assert_capability(var)))
# define __asserts_shared_cap(var)		__attribute__((assert_shared_capability(var)))
# define __returns_cap(var)			__attribute__((lock_returned(var)))

/*
 * The below are used to annotate code being checked.
 */
# define __var_guarded_by(var)		__attribute__((guarded_by(var)))
# define __ref_guarded_by(var)		__attribute__((pt_guarded_by(var)))
# define __excludes_cap(var)		__attribute__((locks_excluded(var)))
# define __requires_cap(var)		__attribute__((requires_capability(var)))
# define __requires_shared_cap(var)	__attribute__((requires_shared_capability(var)))

/*
 * Convenience helper to name a type with capability of the same name.
 * TODO: explain this
 */
# define struct_with_capability(name)									\
	struct __cap_type(name) name;									\
	static __always_inline void __acquire_cap(const struct name *var)				\
		__attribute__((overloadable)) __no_capability_analysis __acquires_cap(var) { }		\
	static __always_inline void __acquire_shared_cap(const struct name *var)			\
		__attribute__((overloadable)) __no_capability_analysis __acquires_shared_cap(var) { }	\
	static __always_inline bool __try_acquire_cap(const struct name *var, bool ret)			\
		__attribute__((overloadable)) __no_capability_analysis __try_acquires_cap(1, var)	\
	{ return ret; }											\
	static __always_inline bool __try_acquire_shared_cap(const struct name *var, bool ret)		\
		__attribute__((overloadable)) __no_capability_analysis __try_acquires_shared_cap(1, var) \
	{ return ret; }											\
	static __always_inline void __release_cap(const struct name *var)				\
		__attribute__((overloadable)) __no_capability_analysis __releases_cap(var) { }		\
	static __always_inline void __release_shared_cap(const struct name *var)			\
		__attribute__((overloadable)) __no_capability_analysis __releases_shared_cap(var) { }	\
	static __always_inline void __assert_cap(const struct name *var)				\
		__attribute__((overloadable)) __asserts_cap(var) { }					\
	static __always_inline void __assert_shared_cap(const struct name *var)				\
		__attribute__((overloadable)) __asserts_shared_cap(var) { }				\
	struct name

/*
 * TODO:
 */
# define disable_capability_analysis()				\
	__diag_push();						\
	__diag_ignore_all("-Wunknown-warning-option", "")	\
	__diag_ignore_all("-Wthread-safety", "")		\
	__diag_ignore_all("-Wthread-safety-addressof", "")

/*
 * TODO:
 */
# define enable_capability_analysis() __diag_pop()

#else /* !WARN_CAPABILITY_ANALYSIS */

# define __no_capability_analysis
# define __cap_type(name)
# define __acquires_cap(var)
# define __acquires_shared_cap(var)
# define __try_acquires_cap(ret, var)
# define __try_acquires_shared_cap(ret, var)
# define __releases_cap(var)
# define __releases_shared_cap(var)
# define __asserts_cap(var)
# define __asserts_shared_cap(var)
# define __returns_cap(var)
# define __var_guarded_by(var)
# define __ref_guarded_by(var)
# define __excludes_cap(var)
# define __requires_cap(var)
# define __requires_shared_cap(var)
# define __acquire_cap(var)			do { } while (0)
# define __acquire_shared_cap(var)		do { } while (0)
# define __try_acquire_cap(var, ret)		(ret)
# define __try_acquire_shared_cap(var, ret)	(ret)
# define __release_cap(var)			do { } while (0)
# define __release_shared_cap(var)		do { } while (0)
# define __assert_cap(var)			do { (void)(var); } while (0)
# define __assert_shared_cap(var)		do { (void)(var); } while (0)
# define struct_with_capability(name)		struct name
# define disable_capability_analysis()
# define enable_capability_analysis()

#endif /* WARN_CAPABILITY_ANALYSIS */

/*
 * TODO: explain
 *
 * Works with any void or non-void expression.
 */
#define capability_unsafe(...)		\
({					\
	disable_capability_analysis();	\
	__VA_ARGS__;			\
	enable_capability_analysis()	\
})

/*
 * An abstract global capability used as a token, but not backed by a real data
 * structure (linker error if accidentally used).
 */
#define token_capability(name)				\
	struct_with_capability(__capability_##name) {};	\
	extern const struct __capability_##name *name
/*
 * To define additional instances of the same token capability.
 */
#define token_capability_instance(cap, name)		\
	extern const struct __capability_##cap *name

/*
 * Common keywords for static capability analysis. Both Clang's capability
 * analysis and Sparse's context tracking are currently supported.
 */
#ifdef __CHECKER__

/* Sparse context/lock checking support. */
# define __must_hold(x)		__attribute__((context(x,1,1)))
# define __must_not_hold(x)
# define __acquires(x)		__attribute__((context(x,0,1)))
# define __cond_acquires(x)	__attribute__((context(x,0,-1)))
# define __releases(x)		__attribute__((context(x,1,0)))
# define __acquire(x)		__context__(x,1)
# define __release(x)		__context__(x,-1)
# define __cond_acquire(x, c)	((c) ? ({ __acquire(x); 1; }) : 0)
/* For Sparse, there's no distinction between exclusive and shared locks. */
# define __must_hold_shared	__must_hold
# define __acquires_shared	__acquires
# define __cond_acquires_shared __cond_acquires
# define __releases_shared	__releases
# define __acquire_shared	__acquire
# define __release_shared	__release
# define __cond_acquire_shared	__cond_acquire

#else /* !__CHECKER__ */

# define __must_hold(x)		__requires_cap(x)
# define __must_not_hold(x)	__excludes_cap(x)
# define __acquires(x)		__acquires_cap(x)
# define __cond_acquires(x)	__try_acquires_cap(1, x)
# define __releases(x)		__releases_cap(x)
# define __acquire(x)		__acquire_cap(x)
# define __release(x)		__release_cap(x)
# define __cond_acquire(x, c)	__try_acquire_cap(x, c)
# define __must_hold_shared(x)	__requires_shared_cap(x)
# define __acquires_shared(x)	__acquires_shared_cap(x)
# define __cond_acquires_shared(x) __try_acquires_shared_cap(1, x)
# define __releases_shared(x)	__releases_shared_cap(x)
# define __acquire_shared(x)	__acquire_shared_cap(x)
# define __release_shared(x)	__release_shared_cap(x)
# define __cond_acquire_shared(x, c) __try_acquire_shared_cap(x, c)

#endif /* __CHECKER__ */

#endif /* _LINUX_COMPILER_CAPABILITY_ANALYSIS_H */
