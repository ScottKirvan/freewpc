/*
 * Copyright 2006, 2007 by Brian Dominy <brian@oddchange.com>
 *
 * This file is part of FreeWPC.
 *
 * FreeWPC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * FreeWPC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FreeWPC; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _SYS_TASK_H
#define _SYS_TASK_H

/*
 * The first section contains common defines that are always available.
 */

extern bool task_dispatching_ok;

/** Values for the 'flags' field */

/* TASK_PROTECTED means that a task is immune to task_kill_gid.
 * It can only exit by means of dying, i.e. task_exit.
 * A task should set this upon entry before it begins anything
 * urgent. */
#define TASK_PROTECTED   0x01



/* Now, the platform specific defines. */

#ifdef CONFIG_PLATFORM_LINUX

#include <pth.h>

typedef pth_t task_pid_t;
typedef unsigned int task_gid_t;
typedef unsigned int task_ticks_t;
typedef void (*task_function_t) (void);
//extern void task_set_rom_page (task_t *pid, U8 rom_page);

#else /* !CONFIG_PLATFORM_LINUX */

#include <env.h>
#ifdef HAVE_LIBC
#include <sys/types.h>
#endif


/** Values for the 'state' field in the task structure */
#define TASK_FREE    0x0
#define TASK_USED    0x1
#define TASK_BLOCKED 0x2
#define TASK_MALLOC  0x4
#define TASK_TASK		0x8
#define TASK_STACK	0x10

/* The TASK_HEAP_SIZE field is not currently used.  It is
 * intended to represent how much memory, in bytes, has been
 * stolen from the stack for task-local variables.  Bytes are
 * always allocated in pairs, so the heap size may be 0, 2,
 * 4, or 6 at most.  The heap area cuts into the stack size
 * as set below in TASK_STACK_SIZE, so it should be used
 * cautiously.  Most tasks will not need any heap storage.
 */
#define TASK_HEAP_SIZE   (0x02 + 0x04)

/** The maximum number of tasks that can be running at once.
 * Space for this many task structures is statically allocated.
 */
#define NUM_TASKS 48

/*
 * Define the size of the saved process stack.
 *
 * This value + 20 should equal a power of 2.
 */
#define TASK_STACK_SIZE		60

/** Type for the group ID (gid) */
typedef U8 task_gid_t;

/** Type for the number of task ticks.  1 task tick is the
 * smallest amount that a task can sleep. */
typedef U8 task_ticks_t;

/** Type for the entry point to a new task */
typedef void (*task_function_t) (void);

/** Type for the task structure.
 * For proper saving/restoring, only the registers that are live
 * across function calls need to be kept here.  The 6809 compiler
 * allows functions to trash D and X, so they aren't here -- just
 * U and Y.
 *
 * When adding/removing fields to this structure, adjust
 * TASK_STACK_SIZE above accordingly so that the total size is
 * a power of 2.
 */
typedef struct task_struct
{
	/** The execution state of the task.  It can be TASK_FREE, if the
	 * task entry isn't being used at all; TASK_USED for a running/waiting
	 * task; or TASK_BLOCKED for a sleeping task. */
	U8				state;

	/** The task group ID.  This is a compile-time assigned value
	 * used to identify the task.   Multiple running tasks can also
	 * share the same group ID; operations on a group will affect
	 * *all* tasks with that ID, in most cases. */
	task_gid_t	gid;

	/** The saved PC for the task, while it is asleep */
	U16			pc;

	/** The saved Y register for the task, while it is asleep */
	U16			y;

	/** The saved U register for the task, while it is asleep */
	U16			u;

	/** The saved ROM page register for the task, while it is asleep */
	U8				rom_page;

	/** The amount of stack space, in bytes, that has been saved into
	 * the task's stack area */
	U8				stack_size;

	/** Miscellaneous control flags */
	U8				flags;

	/** The amount of time that a blocked task has requested to
	sleep */
	U8				delay;

	/** The time at which the task went to sleep.  To see if a task
	needs to be awakened, compute current time - asleep, and if
	see if that is greater than or equal to the intended delay.
	This method handles the 8-bit overflow properly. */
	U8				asleep;

	/** The task argument pointer.  This is a primitive way of passing
	 * initialization data to a new task.  The creator of the task can
	 * assign this argument pointer, after creating the task but before
	 * the next schedule. */
	/* TODO - few  tasks need an arg, and this is wasting RAM for those
	 * that don't.  Better to push any arguments onto the new task's stack
	 * and declare the task function to accept those arguments, with
	 * an ellipsis to prevent assuming they are in registers. */
	PTR_OR_U16	arg;

	/** Thread local data.  Some types of tasks need to maintain local
	 * state information, where each task has its own copy of the data.
	 * The task structure reserves 4 bytes for this purpose; tasks are
	 * free to use this however they choose.   See include/sys/leff.h
	 * for one example of how this is done. */
	/* TODO - should this be overlaid at the bottom of the stack?
	 * Again, most functions won't need this. */
	U8          thread_data[4];

	/** The index of an auxiliary memory block used to store
	 * additional stack data.  This is used when the stack space
	 * within the task block itself is not large enough.  When set
	 * to -1, it means there is no auxiliary storage. */
	U8				aux_stack_block;

	/** The task stack save area.  This is NOT used as the live stack
	 * area; the live stack is copied here when the task blocks.
	 * Because of this, tasks can use a much larger stack size if needed,
	 * as long as they don't try to block while holding that much space.
	 * Practically, this means that you shouldn't sleep in a deeply
	 * nested set of function calls.
	 */
	U8				stack[TASK_STACK_SIZE];
} task_t;


extern task_t *task_current;


/********************************/
/*     Inline Macros            */
/********************************/

extern inline task_t *task_getpid (void)
{
	return task_current;
}

extern inline task_gid_t task_getgid (void)
{
	return task_current->gid;
}

extern inline U8 task_get_thread_data (task_t *pid, U8 n)
{
	return pid->thread_data[n];
}

extern inline void task_set_thread_data (task_t *pid, U8 n, U8 v)
{
	pid->thread_data[n] = v;
}

extern inline void task_set_rom_page (task_t *pid, U8 rom_page)
{
	pid->rom_page = rom_page;
}

/*******************************/
/*     Debug Timing            */
/*******************************/

#define debug_time_start() \
{ \
	U8 __debug_timer = irq_count; \


#define debug_time_stop() \
	dbprintf ("debug time: %02X %02X\n", \
		__debug_timer, irq_count); \
}

/** A process ID, or PID, is just a pointer to the task block.
 * PIDs are rarely used as they are dynamic in value. */
typedef task_t *task_pid_t;

#endif /* CONFIG_PLATFORM_LINUX */


/********************************/
/*     Function Prototypes      */
/********************************/

void task_dump (void);
void task_init (void);
void task_create (void);
void task_inherit_thread_data (task_pid_t tp);
task_pid_t task_create_gid (task_gid_t, task_function_t fn);
task_pid_t task_create_gid1 (task_gid_t, task_function_t fn);
task_pid_t task_recreate_gid (task_gid_t, task_function_t fn);
task_pid_t task_getpid (void);
task_gid_t task_getgid (void);
void task_setgid (task_gid_t gid);
void task_sleep (task_ticks_t ticks);
void task_sleep_sec (int8_t secs);
__noreturn__ void task_exit (void);
task_pid_t task_find_gid (task_gid_t);
task_pid_t task_find_gid_data (task_gid_t gid, U8 off, U8 val);
void task_kill_pid (task_pid_t tp);
bool task_kill_gid (task_gid_t);
void task_kill_all (void);
void task_set_flags (U8 flags);
void task_clear_flags (U8 flags);
PTR_OR_U16 task_get_arg (void);
void task_set_arg (task_pid_t tp, PTR_OR_U16 arg);
__noreturn__ void task_dispatcher (void);
#ifdef CONFIG_PLATFORM_LINUX
task_pid_t task_getpid (void);
task_gid_t task_getgid (void);
U8 task_get_thread_data (task_pid_t pid, U8 n);
void task_set_thread_data (task_pid_t pid, U8 n, U8 v);
#endif

#define task_create_peer(fn)		task_create_gid (task_getgid (), fn)

#define leff_create_peer(fn)     task_inherit_thread_data (task_create_peer(fn))

#define task_create_anon(fn)		task_create_gid (0, fn)

#define task_kill_peers()			task_kill_gid (task_getgid ())

#define task_yield()					task_sleep (0)

extern inline task_pid_t
far_task_create_gid (task_gid_t gid, task_function_t fn, U8 page)
{
	task_pid_t pid = task_create_gid (gid, fn);
	task_set_rom_page (pid, page);
	return pid;
}

extern inline task_pid_t
far_task_create_gid1 (task_gid_t gid, task_function_t fn, U8 page)
{
	task_pid_t pid = task_create_gid1 (gid, fn);
	task_set_rom_page (pid, page);
	return pid;
}

extern inline task_pid_t
far_task_recreate_gid (task_gid_t gid, task_function_t fn, U8 page)
{
	task_pid_t pid = task_recreate_gid (gid, fn);
	task_set_rom_page (pid, page);
	return pid;
}



#ifdef CONFIG_PLATFORM_LINUX

#else /* !CONFIG_PLATFORM_LINUX */

#define TASK_DECL_ARGS(args...) args, ...

#define TASK_GET_ARG(arg) arg

#define task_top_of_stack(tp)	(&tp->stack[TASK_STACK_SIZE - tp->stack_size])

#define task_push_arg(tp, arg) \
do { \
	tp->stack_size += sizeof (arg); \
	*((typeof (arg) *)task_top_of_stack(tp)) = arg; \
} while (0)

#define task_push_args_done(tp) task_push_arg(tp, ((U16)task_exit))

#endif

#endif /* _SYS_TASK_H */
