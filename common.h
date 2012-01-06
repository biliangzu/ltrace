/*
 * This file is part of ltrace.
 * Copyright (C) 2011,2012 Petr Machata, Red Hat Inc.
 * Copyright (C) 2010 Joe Damato
 * Copyright (C) 2009 Juan Cespedes
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef COMMON_H
#define COMMON_H

#include <config.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>

#include "ltrace.h"
#include "defs.h"
#include "dict.h"
#include "sysdep.h"
#include "debug.h"
#include "ltrace-elf.h"
#include "read_config_file.h"
#include "proc.h"
#include "forward.h"

#if defined HAVE_LIBSUPC__ || defined HAVE_LIBSTDC__
# define USE_CXA_DEMANGLE
#endif
#if defined HAVE_LIBIBERTY || defined USE_CXA_DEMANGLE
# define USE_DEMANGLE
#endif

extern char * command;

extern int exiting;  /* =1 if we have to exit ASAP */

typedef struct Function Function;
struct Function {
	const char * name;
	struct param *params;
	struct arg_type_info *return_info;
	size_t num_params;
	Function * next;
};

extern Function * list_of_functions;
extern char *PLTs_initialized_by_here;

struct opt_c_struct {
	int count;
	struct timeval tv;
};

#include "options.h"
#include "output.h"
#ifdef USE_DEMANGLE
#include "demangle.h"
#endif

extern Dict * dict_opt_c;

enum process_status {
	ps_invalid,	/* Failure.  */
	ps_stop,	/* Job-control stop.  */
	ps_tracing_stop,
	ps_sleeping,
	ps_zombie,
	ps_other,	/* Necessary other states can be added as needed.  */
};

/* Events  */
enum ecb_status {
	ecb_cont, /* The iteration should continue.  */
	ecb_yield, /* The iteration should stop, yielding this
		    * event.  */
	ecb_deque, /* Like ecb_stop, but the event should be removed
		    * from the queue.  */
};
extern Event * next_event(void);
extern Event * each_qd_event(enum ecb_status (* cb)(Event * event, void * data),
			     void * data);
extern void enque_event(Event * event);
extern void handle_event(Event * event);

extern pid_t execute_program(const char * command, char ** argv);

extern void show_summary(void);

struct breakpoint;
struct library_symbol;

struct breakpoint;

/* Arch-dependent stuff: */
extern char * pid2name(pid_t pid);
extern pid_t process_leader(pid_t pid);
extern int process_tasks(pid_t pid, pid_t **ret_tasks, size_t *ret_n);
extern int process_stopped(pid_t pid);
extern enum process_status process_status(pid_t pid);
extern void trace_set_options(struct Process *proc);
extern int wait_for_proc(pid_t pid);
extern void trace_me(void);
extern int trace_pid(pid_t pid);
extern void untrace_pid(pid_t pid);
extern void get_arch_dep(Process * proc);
extern void * get_instruction_pointer(Process * proc);
extern void set_instruction_pointer(Process * proc, void * addr);
extern void * get_stack_pointer(Process * proc);
extern void * get_return_addr(Process * proc, void * stack_pointer);
extern void set_return_addr(Process * proc, void * addr);
extern void enable_breakpoint(struct Process *proc, struct breakpoint *sbp);
extern void disable_breakpoint(struct Process *proc, struct breakpoint *sbp);
extern int syscall_p(Process * proc, int status, int * sysnum);
extern void continue_process(pid_t pid);
extern void continue_after_signal(pid_t pid, int signum);
extern void continue_after_syscall(Process *proc, int sysnum, int ret_p);
extern void continue_after_breakpoint(struct Process *proc, struct breakpoint *sbp);
extern void continue_after_vfork(Process * proc);
extern size_t umovebytes (Process *proc, void * addr, void * laddr, size_t count);
extern int ffcheck(void * maddr);
extern void * sym2addr(Process *, struct library_symbol *);
extern int linkmap_init(struct Process *proc, void *dyn_addr);
extern void arch_check_dbg(Process *proc);
extern int task_kill (pid_t pid, int sig);

/* Called when trace_me or primary trace_pid fail.  This may plug in
 * any platform-specific knowledge of why it could be so.  */
void trace_fail_warning(pid_t pid);

/* A pair of functions called to initiate a detachment request when
 * ltrace is about to exit.  Their job is to undo any effects that
 * tracing had and eventually detach process, perhaps by way of
 * installing a process handler.
 *
 * OS_LTRACE_EXITING_SIGHANDLER is called from a signal handler
 * context right after the signal was captured.  It returns 1 if the
 * request was handled or 0 if it wasn't.
 *
 * If the call to OS_LTRACE_EXITING_SIGHANDLER didn't handle the
 * request, OS_LTRACE_EXITING is called when the next event is
 * generated.  Therefore it's called in "safe" context, without
 * re-entrancy concerns, but it's only called after an event is
 * generated.  */
int os_ltrace_exiting_sighandler(void);
void os_ltrace_exiting(void);

int arch_elf_init(struct ltelf *lte, struct library *lib);
void arch_elf_destroy(struct ltelf *lte);

enum plt_status {
	plt_fail,
	plt_ok,
	plt_default,
};

enum plt_status arch_elf_add_plt_entry(struct Process *p, struct ltelf *l,
				       const char *n, GElf_Rela *r, size_t i,
				       struct library_symbol **ret);

int arch_breakpoint_init(struct Process *proc, struct breakpoint *sbp);
void arch_breakpoint_destroy(struct breakpoint *sbp);
int arch_breakpoint_clone(struct breakpoint *retp, struct breakpoint *sbp);

void arch_library_init(struct library *lib);
void arch_library_destroy(struct library *lib);
void arch_library_clone(struct library *retp, struct library *lib);

int arch_library_symbol_init(struct library_symbol *libsym);
void arch_library_symbol_destroy(struct library_symbol *libsym);
int arch_library_symbol_clone(struct library_symbol *retp,
			      struct library_symbol *libsym);

int arch_process_init(struct Process *proc);
void arch_process_destroy(struct Process *proc);
int arch_process_clone(struct Process *retp, struct Process *proc);
int arch_process_exec(struct Process *proc);

/* This is called after the dynamic linker is done with the
 * process startup.  */
void arch_dynlink_done(struct Process *proc);

/* Format VALUE into STREAM.  The dictionary of all arguments is given
 * for purposes of evaluating array lengths and other dynamic
 * expressions.  Returns number of characters outputted, -1 in case of
 * failure.  */
int format_argument(FILE *stream, struct value *value,
		    struct value_dict *arguments);

#endif
