#include <linux/kernel.h>
#include <linux/module.h>
// #include <linux/task.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Impervguin");
MODULE_DESCRIPTION("Module for getting proc info");

#define BUFFER_SIZE 16

static void parse_exit_state(char buf[BUFFER_SIZE], unsigned int state) {
    if (state == 0) {
        sprintf(buf, "No Exit(0)");
    }
    if (state & EXIT_DEAD) {
        sprintf(buf, "Dead");
    }
    if (state & EXIT_ZOMBIE) {
        sprintf(buf, "Zombie");
    }
}

static void print_task_flags(unsigned int flags)
{
    printk(KERN_CONT " flags=(");
    
    if (flags & PF_IDLE) printk(KERN_CONT "IDLE,");
    if (flags & PF_EXITING) printk(KERN_CONT "EXITING, ");
    if (flags & PF_KTHREAD) printk(KERN_CONT "KTHREAD, ");
    if (flags & PF_RANDOMIZE) printk(KERN_CONT "RANDOMIZE, ");
    if (flags & PF_KSWAPD) printk(KERN_CONT "KSWAPD, ");

    if (flags & PF_VCPU) printk(KERN_CONT "VCPU, ");
    if (flags & PF_WQ_WORKER) printk(KERN_CONT "WQ_WORKER, ");
    if (flags & PF_FORKNOEXEC) printk(KERN_CONT "FORKNOEXEC, ");
    if (flags & PF_MCE_PROCESS) printk(KERN_CONT "MCE_PROCESS, ");
    if (flags & PF_SUPERPRIV) printk(KERN_CONT "SUPERPRIV, ");
    if (flags & PF_DUMPCORE) printk(KERN_CONT "DUMPCORE, ");
    if (flags & PF_SIGNALED) printk(KERN_CONT "SIGNALED, ");
    if (flags & PF_MEMALLOC) printk(KERN_CONT "MEMALLOC, ");
    if (flags & PF_NPROC_EXCEEDED) printk(KERN_CONT "NPROC_EXCEEDED, ");
    if (flags & PF_USED_MATH) printk(KERN_CONT "USED_MATH, ");
    if (flags & PF_NOFREEZE) printk(KERN_CONT "NOFREEZE, ");
    if (flags & PF_KSWAPD) printk(KERN_CONT "KSWAPD, ");
    if (flags & PF_MEMALLOC_NOIO) printk(KERN_CONT "MEMALLOC_NOIO, ");
    if (flags & PF_NO_SETAFFINITY) printk(KERN_CONT "PF_NO_SETAFFINITY, ");
    
    printk(KERN_CONT ")");
}

static void parse_policy(char buf[BUFFER_SIZE], int policy) {
    switch (policy) {
        case SCHED_NORMAL:
            sprintf(buf, "Normal(%d)", SCHED_NORMAL);
            break;
        case SCHED_FIFO:
            sprintf(buf, "FIFO(%d)", SCHED_FIFO);
            break;
        case SCHED_RR:
            sprintf(buf, "Round-robin(%d)", SCHED_RR);
            break;
        case SCHED_BATCH:
            sprintf(buf, "Batch(%d)", SCHED_BATCH);
            break;
        case SCHED_DEADLINE:
            sprintf(buf, "Deadline(%d)", SCHED_DEADLINE);
            break;
        case SCHED_IDLE:
            sprintf(buf, "Idle(%d)", SCHED_IDLE);
            break;
        default:
            sprintf(buf, "undefined(%d)", policy);
            break;
    }
}

static __init int procinfo_init(void) {
    printk("procinfo init.\n");
    struct task_struct *task = &init_task;
    do
    {
        char policy[BUFFER_SIZE];
        parse_policy(policy, task->policy);
        char exit_state[BUFFER_SIZE];
        parse_exit_state(exit_state, task->exit_state);
        printk("pid=%d ppid=%d comm=%s pcomm=%s state=%d state=%c prio=%d normal_prio=%d static_prio=%d rt_prio=%d policy=%s exit_state=%s exit_code=%d exit_signal=%d core_occupation=%d utime=%llu stime=%llu pages_to_reclaim=%d migration_disabled=%d migration_flags=%d ",
            task->pid,
            task->parent->pid,
            task->comm,
            task->parent->comm,
            task->__state,
            task_state_to_char(task),
            task->prio,
            task->normal_prio,
            task->static_prio,
            task->rt_priority,
            policy,
            exit_state,
            task->exit_code,
            task->exit_signal,
            task->core_occupation,
            task->utime,
            task->stime,
            task->memcg_nr_pages_over_high,
            task->migration_disabled,
            task->migration_flags
        );
        print_task_flags(task->flags);
        printk("\n");
    } while ((task = next_task(task)) != &init_task);
    return 0;
}

static __exit void procinfo_exit(void) {
    printk("procinfo exit.\n");
}

module_init(procinfo_init);
module_exit(procinfo_exit);