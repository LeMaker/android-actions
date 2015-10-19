/*
 * Copyright (C) 2012 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM lowmem_killer

#if !defined(_LOWMEM_KILLER_H) || defined(TRACE_HEADER_MULTI_READ)
#define _LOWMEM_KILLER_H
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/swap.h>
#include <linux/rcupdate.h>
#include <linux/notifier.h>

#include <linux/tracepoint.h>

TRACE_EVENT(lowmem_kill,

	TP_PROTO(struct task_struct *selected,   int other_file,  int minfree, int min_score_adj,  int other_free),

	TP_ARGS(selected, other_file, minfree, min_score_adj, other_free),

	TP_STRUCT__entry(
		__field( unsigned int,		other_file			)
		__field( unsigned int,		minfree			)
		__field( unsigned int,		min_score_adj			)
		__field( unsigned int,		other_free			)
		__field( unsigned int,		oom_adj			)
		__array( char,		selected_comm,	TASK_COMM_LEN	)
		__field( unsigned int,	selected_pid		)
		__array( char,      cur_comm, TASK_COMM_LEN )
		__field( unsigned int,	cur_pid		)
        ),

	TP_fast_assign(							
		memcpy(__entry->selected_comm, selected->comm, TASK_COMM_LEN);
		__entry->selected_pid	= selected->pid;		
		__entry->oom_adj		= selected->signal->oom_score_adj;
		memcpy(__entry->cur_comm, current->comm, TASK_COMM_LEN);
		__entry->cur_pid	= current->pid;
		__entry->other_file	= other_file * (long)(PAGE_SIZE / 1024);
		__entry->minfree	= minfree * (long)(PAGE_SIZE / 1024);
		__entry->min_score_adj	= min_score_adj;
		__entry->other_free	= other_free * (long)(PAGE_SIZE / 1024);
        ),

	TP_printk("selected [%s] %d  oomadj %u,  current [%s] %d,  other_file %d other_free %d minfree %d min_score_adj %d",
		  __entry->selected_comm,__entry->selected_pid,__entry->oom_adj,
		  __entry->cur_comm,__entry->cur_pid,
		  __entry->other_file, __entry->other_free, __entry->minfree, __entry->min_score_adj)
);


#endif

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE lowmem_killer
#include <trace/define_trace.h>
