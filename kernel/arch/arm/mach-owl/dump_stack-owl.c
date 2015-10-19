#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <linux/kallsyms.h>

#include <linux/nsproxy.h>
#include <linux/sched.h>
#include <linux/stacktrace.h>
#include <asm/stacktrace.h>
#include <linux/slab.h>

static int show_pid = 1;

struct stack_trace_data {
    struct stack_trace *trace;
    unsigned int no_sched_functions;
    unsigned int skip;
};

static int save_trace(struct stackframe *frame, void *d)
{
    struct stack_trace_data *data = d;
    struct stack_trace *trace = data->trace;
    unsigned long addr = frame->pc;

//  if (data->no_sched_functions && in_sched_functions(addr))
//      return 0;
    if (data->skip) {
        data->skip--;
        return 0;
    }

    trace->entries[trace->nr_entries++] = addr;

    return trace->nr_entries >= trace->max_entries;
}

static void save_stack_trace_tsk2(struct task_struct *tsk, struct stack_trace *trace)
{
    struct stack_trace_data data;
    struct stackframe frame;

    data.trace = trace;
    data.skip = trace->skip;

    if (tsk != current) {
//#ifdef CONFIG_SMP
        /*
         * What guarantees do we have here that 'tsk' is not
         * running on another CPU?  For now, ignore it as we
         * can't guarantee we won't explode.
         */
//      if (trace->nr_entries < trace->max_entries)
//          trace->entries[trace->nr_entries++] = ULONG_MAX;
//      return;
//#else
        data.no_sched_functions = 1;
        frame.fp = thread_saved_fp(tsk);
        frame.sp = thread_saved_sp(tsk);
        frame.lr = 0;       /* recovered from the stack */
        frame.pc = thread_saved_pc(tsk);
//#endif
    } else {
        register unsigned long current_sp asm ("sp");

        data.no_sched_functions = 0;
        frame.fp = (unsigned long)__builtin_frame_address(0);
        frame.sp = current_sp;
        frame.lr = (unsigned long)__builtin_return_address(0);
        frame.pc = (unsigned long)save_stack_trace_tsk2;
    }

    walk_stackframe(&frame, save_trace, &data);
    if (trace->nr_entries < trace->max_entries)
        trace->entries[trace->nr_entries++] = ULONG_MAX;
}


static int dump_task_stack(struct task_struct *task)
{
    struct stack_trace trace;
    char *buf;

    printk(KERN_INFO "dump_task_stack: %p\n", task);

    buf = kmalloc(4096, GFP_KERNEL);
    trace.nr_entries    = 0;
    trace.skip      = 0;
    trace.entries       = (unsigned long*)buf;
    trace.max_entries   = 100; 

    save_stack_trace_tsk2(task, &trace);
    print_stack_trace(&trace, 4);

    kfree(buf);

    return 0;
}

static inline void my_show_state(long state)
{
    /* -1 unrunnable, 0 runnable, >0 stopped */
    if(state==0)
       printk(" runnable\n");
    else if(state>0)
       printk(" stopped\n");
    else
       printk(" unrunnable\n");
}

static int dump_all_tasks(void)
{
#if 1
    struct task_struct *g, *p;
    int count=0;

    printk( "Module testcurrent init\n" );
    do_each_thread(g, p) {
        count++;
        printk( "%d : Thread name %s ,pid=%6d ",count ,p->comm,p->pid);
        my_show_state(p->state);
        dump_task_stack(p);
    } while_each_thread(g, p);
#else   
    struct task_struct *task;

    struct list_head *list;
    int count=0;
    printk( "Module testcurrent init\n" );

    list_for_each(list, &current->tasks)
    { 
       count++;
       task=list_entry(list,struct task_struct, tasks);
       printk( "%d : Thread name %s ,pid=%6d ",count ,task->comm,task->pid);
       my_show_state(task->state);

       dump_task_stack(task);
    }
    printk( " current is %s ,total %d\n",current->comm,count );
#endif
    return 0;
}
static int dump_cur_task(int new_pid)
{
    struct task_struct *p; 
    struct nsproxy *nsproxy = get_current()->nsproxy;

    p =pid_task(find_pid_ns(show_pid, nsproxy->pid_ns), PIDTYPE_PID);
    if(p == NULL)
    {
            printk("can not find task pid: %d\n", show_pid);
            return -EINVAL;
    }   
    printk("find task: name %s, pid: %d, leader name %s pid: %d\n", 
             p->comm, p->pid, p->group_leader->comm, p->group_leader->pid);
    my_show_state(p->state);
   
    sched_show_task(p);
    return 0;
}

static ssize_t sstack_read_proc(struct file *file, char __user *buffer,
                   size_t count, loff_t * offset)
{
    dump_cur_task(show_pid);

    return 0;
}

static ssize_t sstack_write_proc(struct file *file, const char __user *buffer,
                   size_t count, loff_t * offset)
{
    char buf[16];
    int new_pid;

    if(count > 16) {
        printk(KERN_INFO "the count is too large");
        return -EINVAL;
    }

    if(copy_from_user(buf, buffer, count)) {
        return -EFAULT;
    }
    new_pid = simple_strtoul(buf, NULL, 10);
    show_pid = new_pid;
    if(show_pid != 0) {
        int ret;

        ret = dump_cur_task(new_pid);
        if(ret) 
            return ret;
        else
            return count;
    } else {
        dump_all_tasks();
        return count;
    }
}

static const struct file_operations __sstack_file_operations =
{
    .owner =        THIS_MODULE,
    .read =         sstack_read_proc,
    .write =        sstack_write_proc,
};

static void sstack_create_proc(void)
{
    proc_create("show_stack", S_IRUGO | S_IWUGO, NULL, &__sstack_file_operations);
}

static int __init sstack_init(void)
{
    sstack_create_proc();
    return 0;
}

late_initcall(sstack_init);
