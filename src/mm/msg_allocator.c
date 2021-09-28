/**
 * @file mm/msg_allocator.c
 *
 * @brief Memory management functions for messages
 *
 * Memory management functions for messages
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/msg_allocator.h>

#include <core/core.h>
#include <datatypes/array.h>
#include <gvt/gvt.h>

static __thread dyn_array(struct lp_msg *) free_list = {0};

void msg_allocator_init(void)
{
	array_init(free_list);
}

void msg_allocator_fini(void)
{
	while (!array_is_empty(free_list)) {
		mm_free(array_pop(free_list));
	}
	array_fini(free_list);
}

struct lp_msg* msg_allocator_alloc(unsigned payload_size)
{
	struct lp_msg *ret;
	if (unlikely(payload_size > BASE_PAYLOAD_SIZE)) {
		ret = mm_alloc(
			offsetof(struct lp_msg, extra_pl) +
			(payload_size - BASE_PAYLOAD_SIZE)
		);
		ret->pl_size = payload_size;
		return ret;
	}
	if (unlikely(array_is_empty(free_list))) {
		ret = mm_alloc(sizeof(struct lp_msg));
		ret->pl_size = payload_size;
		return ret;
	}
	return array_pop(free_list);
}

void msg_allocator_free(struct lp_msg *msg)
{
	if(likely(msg->pl_size <= BASE_PAYLOAD_SIZE))
		array_push(free_list, msg);
	else
		mm_free(msg);
}

extern struct lp_msg* msg_allocator_pack(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size);
