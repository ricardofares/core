/**
 * @file lp/common.h
 *
 * @brief Common LP functionalities
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <arch/timer.h>
#include <lp/lp.h>
#include <lp/msg.h>
#include <log/log.h>
#include <log/stats.h>

static inline void common_msg_process(const struct lp_ctx *lp, const struct lp_msg *msg)
{
	timer_uint t, tv;
	t = timer_hr_new();
	global_config.dispatcher(msg->dest, msg->dest_t, msg->m_type, msg->pl, msg->pl_size, lp->state_pointer);
	tv = timer_hr_value(t);
	stats_take(STATS_MSG_PROCESSED_TIME, tv);
	stats_take(STATS_MSG_PROCESSED, 1);
#ifdef LP_STATS
	// A way to drop the `lp` constness.
	struct lp_stats *stats = (struct lp_stats *)&lp->stats;
	stats->ev_proc_time += tv;
#endif // LP_STATS
}
