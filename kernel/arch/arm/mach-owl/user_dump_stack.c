#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/list.h>

#include <asm/stacktrace.h>
#include <asm/traps.h>
#include <asm/unwind.h>

#ifdef CONFIG_ARM_UNWIND

void user_dump_backtrace_entry(int (*print)(const char *fmt, ...), unsigned long where, unsigned long from, unsigned long frame)
{
#ifdef CONFIG_KALLSYMS
    print("[<%08lx>] (%pS) from [<%08lx>] (%pS)\n", where, (void *)where, from, (void *)from);
#else
    print("Function entered at [<%08lx>] from [<%08lx>]\n", where, from);
#endif
}

void user_unwind_backtrace(int (*print)(const char *fmt, ...), struct pt_regs *regs, struct task_struct *tsk)
{
	struct stackframe frame;
	register unsigned long current_sp asm ("sp");

	if (!tsk)
		tsk = current;

	if (regs) {
		frame.fp = regs->ARM_fp;
		frame.sp = regs->ARM_sp;
		frame.lr = regs->ARM_lr;
		/* PC might be corrupted, use LR in that case. */
		frame.pc = kernel_text_address(regs->ARM_pc)
			 ? regs->ARM_pc : regs->ARM_lr;
	} else if (tsk == current) {
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.sp = current_sp;
		frame.lr = (unsigned long)__builtin_return_address(0);
		frame.pc = (unsigned long)unwind_backtrace;
	} else {
		/* task blocked in __switch_to */
		frame.fp = thread_saved_fp(tsk);
		frame.sp = thread_saved_sp(tsk);
		/*
		 * The function calling __switch_to cannot be a leaf function
		 * so LR is recovered from the stack.
		 */
		frame.lr = 0;
		frame.pc = thread_saved_pc(tsk);
	}

	while (1) {
		int urc;
		unsigned long where = frame.pc;

		urc = unwind_frame(&frame);
		if (urc < 0)
			break;
		user_dump_backtrace_entry(print, where, frame.pc, frame.sp - 4);
	}
}

static inline void user_dump_backtrace(int (*print)(const char *fmt, ...), struct pt_regs *regs, struct task_struct *tsk)
{
	user_unwind_backtrace(print, regs, tsk);
}

void user_dump_stack(int (*print)(const char *fmt, ...), struct task_struct *tsk)
{
	user_dump_backtrace(print, NULL, tsk);
}
EXPORT_SYMBOL_GPL(user_dump_stack);

#endif
