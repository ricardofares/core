/**
* @file test/test.h
*
* @brief Custom minimalistic testing framework
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#include <stdio.h>
#include <setjmp.h>
#include <assert.h>
#include <limits.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
typedef HANDLE os_semaphore;
#elif defined(__APPLE__) && defined(__MACH__)
#include <unistd.h>
#include <mach/mach.h>
typedef semaphore_t os_semaphore;
#elif defined(__unix__) || defined(__unix)
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
typedef sem_t * os_semaphore;
#else
#error Unsupported operating system
#endif

#if defined(__unix__) || defined(__unix) || defined(__APPLE__) && defined(__MACH__)
#define _GNU_SOURCE
#include <pthread.h>

#define THREAD_CALL_CONV
typedef void * thrd_ret_t;
typedef pthread_t thr_id_t;

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#define THREAD_CALL_CONV WINAPI
typedef DWORD thrd_ret_t;
typedef HANDLE thr_id_t;

#define THREAD_RET_FAILURE (1)
#define THREAD_RET_SUCCESS (0)

#else
#error Unsupported operating system
#endif

#ifndef __unused
#define __unused __attribute__((unused))
#endif

typedef thrd_ret_t(*thr_run_fnc)(void *);

struct worker {
	thr_id_t tid;
	unsigned rid;
	int ret;
};

typedef int test_ret_t;
typedef test_ret_t(*test_fn)(void *);

struct test_unit {
	unsigned n_th;
	struct worker *pool;
	jmp_buf fail_buffer;
	test_ret_t ret;
	unsigned total;
	unsigned passed;
	unsigned failed;
	unsigned xfailed;
	unsigned uxpassed;
	unsigned should_pass;
	unsigned should_fail;
};

extern test_ret_t test_thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg);
extern test_ret_t test_thread_wait(thr_id_t thr, thrd_ret_t *ret);

extern struct test_unit test_unit;

extern void spawn_worker_pool(unsigned n_th);
int signal_new_thread_action(test_fn fn, void *args);
void tear_down_worker_pool(void);

extern os_semaphore sema_init(unsigned tokens);
extern void sema_remove(os_semaphore sema);
extern void sema_wait(os_semaphore sema, unsigned count);
extern void sema_signal(os_semaphore sema, unsigned count);

/// The type of this pseudo random number generator state
typedef __uint128_t test_rng_state;

extern int ks_test(__uint32_t N, __uint32_t nBins, double (*sample)(void));
extern __uint64_t lcg_random_u(test_rng_state *rng_state);
extern double lcg_random(test_rng_state *rng_state);
extern __uint64_t lcg_random_range(test_rng_state *rng_state, __uint64_t n);
extern void lcg_init(test_rng_state *rng_state, test_rng_state initseq);
extern __uint64_t test_random_range(__uint64_t n);
extern __uint64_t test_random_u(void);
extern double test_random_double(void);
extern void test_random_init(void);


/****+ API TO BE USED IN TESTS ARE DECLARED BELOW THIS LINE ******/

#define test_assert(condition)                                                                                              \
        do {                                                                                                           \
                if(!(condition)) {                                                                                     \
                        fprintf(stderr, "assertion failed: " #condition " at %s:%d\n", __FILE__, __LINE__);            \
                        test_unit.ret = -1;                                                                            \
                }                                                                                                      \
        } while(0)

#define check_passed_asserts()                                                                                         \
        do {                                                                                                           \
                test_ret_t ret = test_unit.ret;                                                                        \
                test_unit.ret = 0;                                                                                     \
                return ret;                                                                                            \
        } while(0)

#define test_thread_pool_size() (test_unit.n_th)

extern void finish(void);
extern void init(unsigned n_th);
extern void fail(void);
extern void test(char *desc, test_fn test_fn, void *arg);
extern void test_xf(char *desc, test_fn test_fn, void *arg);
extern void parallel_test(char *desc, test_fn test_fn, void *args);
