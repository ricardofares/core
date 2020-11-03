/**
 * @file lp/lp.h
 *
 * @brief LP construction functions
 *
 * LP construction functions
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://hpdcs.github.io
 *
 * This file is part of ROOT-Sim (ROme OpTimistic Simulator).
 *
 * ROOT-Sim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; only version 3 of the License applies.
 *
 * ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once

#include <core/core.h>
#include <gvt/termination.h>
#include <lib/lib.h>
#include <lp/msg.h>
#include <lp/process.h>
#include <mm/model_allocator.h>

struct _lp_struct {
	simtime_t t_d;
	struct lib_state_managed *lsm_p;
	struct process_data p;
	struct mm_state mm_state;		//!< The memory allocator state for this LP
};

typedef struct _lp_struct lp_struct;

#define lps_iter_init(start_i, lps_cnt)					\
{									\
	const unsigned __i = rid;					\
	const unsigned __ucnt = n_threads;				\
	const uint64_t __lpscnt = (nid + 1 == n_nodes) ? 		\
		n_lps - n_lps_node * (n_nodes - 1) : n_lps_node;	\
	const bool __big = __i >= (__ucnt - (__lpscnt % __ucnt));	\
	start_i = n_lps_node * nid;					\
	start_i += __big ?						\
		__lpscnt - ((__lpscnt / __ucnt + 1) * (__ucnt - __i)) : \
		((__lpscnt / __ucnt) * __i);				\
	lps_cnt	= (__lpscnt / __ucnt) + __big;				\
}

extern uint64_t n_lps_node;
extern __thread lp_struct *current_lp;
extern lp_struct *lps;
extern unsigned *lid_to_rid;

extern void lp_global_init(void);
extern void lp_global_fini(void);

extern void lp_init(void);
extern void lp_fini(void);

extern void lp_cleanup(void);
