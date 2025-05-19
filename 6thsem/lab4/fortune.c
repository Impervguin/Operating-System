#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/pid.h>

#define DIRNAME "fortune"
#define FILENAME "fortune_src"
#define SYMNAME "fortune_sym"

#define BUF_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Impervguin");
MODULE_DESCRIPTION("Module for getting task info by pid");

static struct proc_dir_entry *dir;
static struct proc_dir_entry *file;
static struct proc_dir_entry *sym;

static char fortune_buf[BUF_SIZE];
static int fpid = -1;

#define PARSE_SIZE 16

static void parse_exit_state(char buf[PARSE_SIZE], unsigned int state) {
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

static void print_task_flags(unsigned int flags, char *buf, size_t buf_size)
{
    size_t offset = 0;
    
    offset += snprintf(buf + offset, buf_size - offset, "(");
    
    if (flags & PF_IDLE) offset += snprintf(buf + offset, buf_size - offset, "IDLE,");
    if (flags & PF_EXITING) offset += snprintf(buf + offset, buf_size - offset, "EXITING,");
    if (flags & PF_KTHREAD) offset += snprintf(buf + offset, buf_size - offset, "KTHREAD,");
    if (flags & PF_RANDOMIZE) offset += snprintf(buf + offset, buf_size - offset, "RANDOMIZE,");
    if (flags & PF_KSWAPD) offset += snprintf(buf + offset, buf_size - offset, "KSWAPD,");
    if (flags & PF_VCPU) offset += snprintf(buf + offset, buf_size - offset, "VCPU,");
    if (flags & PF_WQ_WORKER) offset += snprintf(buf + offset, buf_size - offset, "WQ_WORKER,");
    if (flags & PF_FORKNOEXEC) offset += snprintf(buf + offset, buf_size - offset, "FORKNOEXEC,");
    if (flags & PF_MCE_PROCESS) offset += snprintf(buf + offset, buf_size - offset, "MCE_PROCESS,");
    if (flags & PF_SUPERPRIV) offset += snprintf(buf + offset, buf_size - offset, "SUPERPRIV,");
    if (flags & PF_DUMPCORE) offset += snprintf(buf + offset, buf_size - offset, "DUMPCORE,");
    if (flags & PF_SIGNALED) offset += snprintf(buf + offset, buf_size - offset, "SIGNALED,");
    if (flags & PF_MEMALLOC) offset += snprintf(buf + offset, buf_size - offset, "MEMALLOC,");
    if (flags & PF_NPROC_EXCEEDED) offset += snprintf(buf + offset, buf_size - offset, "NPROC_EXCEEDED,");
    if (flags & PF_USED_MATH) offset += snprintf(buf + offset, buf_size - offset, "USED_MATH,");
    if (flags & PF_NOFREEZE) offset += snprintf(buf + offset, buf_size - offset, "NOFREEZE,");
    if (flags & PF_KSWAPD) offset += snprintf(buf + offset, buf_size - offset, "KSWAPD,");
    if (flags & PF_MEMALLOC_NOIO) offset += snprintf(buf + offset, buf_size - offset, "MEMALLOC_NOIO,");
    if (flags & PF_NO_SETAFFINITY) offset += snprintf(buf + offset, buf_size - offset, "PF_NO_SETAFFINITY,");
    
    offset += snprintf(buf + offset, buf_size - offset, ")");
    if (offset > strlen("(")) {
        buf[offset-1] = '\0'; 
    }
}

static void parse_policy(char buf[PARSE_SIZE], int policy) {
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

static ssize_t fortune_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    struct task_struct *task;
    int len;
    printk("+ fortune: read, f_pos=%lld, count=%zu\n", *f_pos, count);
    if (*f_pos > 0 || fpid == -1) {
        return 0;
    }
    if (count >= BUF_SIZE) {
        count = BUF_SIZE -1;
    }

    task = pid_task(find_vpid(fpid), PIDTYPE_PID);
    if (task == NULL) {
        len = sprintf(fortune_buf, "No such pid %d", fpid);
        if (copy_to_user(buf, fortune_buf, len))
            return -EFAULT;
        *f_pos += len;
        return len;
    }

    char policy[PARSE_SIZE];
    parse_policy(policy, task->policy);
    char exit_state[PARSE_SIZE];
    parse_exit_state(exit_state, task->exit_state);
    char flags[BUF_SIZE];
    print_task_flags(task->flags, flags, BUF_SIZE);
    len = snprintf(fortune_buf, BUF_SIZE, 
        "pid=%d\n"
        "ppid=%d\n"
        "comm=%s\n"
        "pcomm=%s\n"
        "state=%d\n"
        "state=%c\n"
        "prio=%d\n"
        "normal_prio=%d\n"
        "static_prio=%d\n"
        "rt_prio=%d\n"
        "policy=%s\n" 
        "exit_state=%s\n"
        "exit_code=%d\n" 
        "exit_signal=%d\n"
        "core_occupation=%d\n"
        "utime=%llu\n"
        "stime=%llu\n"
        "pages_to_reclaim=%d\n"
        "migration_disabled=%d\n"
        "migration_flags=%d\n"
        "flags=%s\n",
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
        task->migration_flags,
        flags
    );

    if (copy_to_user(buf, fortune_buf, len))
        return -EFAULT;
    *f_pos += len;
    return len;
}

static ssize_t fortune_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    printk("+ fortune: write, f_pos=%lld, count=%zu\n", *f_pos, count);
    if (*f_pos > 0) {
        return 0;
    }

    char ubuf[PARSE_SIZE];
    if (copy_from_user(ubuf, buf, count))
        return -EFAULT;

    if (kstrtoint(ubuf, 10, &fpid))
        return -EINVAL;

    return count;
}

static int proc_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "fortune: open\n");
    return 0;
}

static int proc_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "fortune: release\n");
    return 0;
}

static const struct proc_ops proc_fops = {
    .proc_open = proc_open,
    .proc_release = proc_release,
    .proc_read = fortune_read,
    .proc_write = fortune_write,
};

static int __init fortune_init(void) {
    printk("+ fortune: init\n");
    dir = proc_mkdir(DIRNAME, NULL);
    if (dir == NULL) {
        printk(KERN_ERR "fortune: proc_mkdir failed\n");
        return -ENOMEM;
    }
    file = proc_create(FILENAME, 0666, dir, &proc_fops);
    if (file == NULL) {
        printk(KERN_ERR "fortune: proc_create failed\n");
        proc_remove(file);
        return -ENOMEM;
    }
    sym = proc_symlink(SYMNAME, NULL, DIRNAME "/" FILENAME);
    if (sym == NULL) {
        printk(KERN_ERR "fortune: proc_create failed\n");
        proc_remove(file);
        proc_remove(dir);
        return -ENOMEM;
    }
    return 0;
}

static void __exit fortune_exit(void) {
    printk("+ fortune: exit\n");
    proc_remove(sym);
    proc_remove(file);
    proc_remove(dir);
}

module_init(fortune_init);
module_exit(fortune_exit);

