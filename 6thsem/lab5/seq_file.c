#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/vmalloc.h>


#define DIRNAME "seqfile"
#define FILENAME "seqfile_src"
#define SYMNAME "seqfile_sym"

#define BUF_SIZE 1024
#define PID_BUF_SIZE 16

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Impervguin");
MODULE_DESCRIPTION("Module for getting task info by pid via seq_file");

static struct proc_dir_entry *dir;
static struct proc_dir_entry *file;
static struct proc_dir_entry *sym;

static char seqfile_buf[BUF_SIZE];
static int pidarr[PID_BUF_SIZE];
static int pidarr_count = 0;

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

static void *my_seq_start(struct seq_file *s, loff_t *pos) {
    printk("+ seqfile: start, pos=%p,  seq=%p\n", pos, s);

    if (*pos >= pidarr_count) {
        *pos = 0;
        return NULL;
    }

    return pidarr + *pos;
}

static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos) {
    if (v == NULL) {
        printk("+ seqfile: next, pos=%p, seq=%p, v is NULL\n", pos, s);
    } else {
        printk("+ seqfile: next, pos=%p,  seq=%p, v=%p, v=%d\n", pos, s, v, *(int *)v);
    }

    (*pos)++;

    if (*pos >= pidarr_count) {
        return NULL;
    }
    
    return pidarr + *pos;
}

static void my_seq_stop(struct seq_file *s, void *v) {
    if (v == NULL) {
        printk("+ seqfile: stop, seq=%p, v is NULL\n", s);
    } else {
        printk("+ seqfile: stop, seq=%p, v is %p\n", s, v);
    }
}


static int my_seq_show(struct seq_file *s, void *v) {
    if (v == NULL) {
        printk("+ seqfile: show, seq=%p, v is NULL\n", s);
    } else {
        printk("+ seqfile: show, seq=%p, v=%p, v=%d\n", s, v, *(int *)v);
    }

    if (v == NULL) {
        return 0;
    }

    int nr = *(int *)v;

    struct task_struct *task;
    struct pid *pid = find_vpid(nr);
    task = pid_task(pid, PIDTYPE_PID);
    if (task == NULL) {
        seq_printf(s, "No such pid %d\n", pid);
        return 0;
    }

    char policy[PARSE_SIZE];
    parse_policy(policy, task->policy);
    char exit_state[PARSE_SIZE];
    parse_exit_state(exit_state, task->exit_state);
    char flags[BUF_SIZE];
    print_task_flags(task->flags, flags, BUF_SIZE);
    snprintf(seqfile_buf, BUF_SIZE, 
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

    seq_printf(s, "%s\n", seqfile_buf);
    return 0;
}


static const struct seq_operations my_seq_ops = {
    .start = my_seq_start,
    .next = my_seq_next,
    .stop = my_seq_stop,
    .show = my_seq_show,
};


static int seq_proc_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "seqfile: open\n");

    return seq_open(file, &my_seq_ops);
}


static ssize_t seq_proc_read(struct file *f, char __user *c, size_t s, loff_t *off)
{
    printk(KERN_INFO "+: seq_file: read\n");
    return seq_read(f, c, s, off);
}

static ssize_t seq_proc_write(struct file *f, const char __user *c, size_t count, loff_t *f_pos)
{
    printk("+ seqfile: write, f_pos=%lld, count=%zu\n", *f_pos, count);
    if (*f_pos > 0) {
        return 0;
    }

    char ubuf[PARSE_SIZE];
    if (copy_from_user(ubuf, c, count))
        return -EFAULT;

    if (kstrtoint(ubuf, 10, pidarr + pidarr_count))
        return -EINVAL;
    
    pidarr_count++;

    return count;
}

static int seq_proc_release(struct inode *inode, struct file *file)
{
    printk("+ seqfile: release\n");
    return seq_release(inode, file);
}

static const struct proc_ops seq_proc_fops = {
    .proc_open = seq_proc_open,
    .proc_release = seq_proc_release,
    .proc_read = seq_proc_read,
    .proc_write = seq_proc_write,
};


static int __init seqfile_init(void) {
    printk("+ seqfile: init\n");
    dir = proc_mkdir(DIRNAME, NULL);
    if (dir == NULL) {
        printk(KERN_ERR "seqfile: proc_mkdir failed\n");
        return -ENOMEM;
    }
    file = proc_create(FILENAME, 0666, dir, &seq_proc_fops);
    if (file == NULL) {
        printk(KERN_ERR "seqfile: proc_create failed\n");
        proc_remove(file);
        return -ENOMEM;
    }
    sym = proc_symlink(SYMNAME, NULL, DIRNAME "/" FILENAME);
    if (sym == NULL) {
        printk(KERN_ERR "seqfile: proc_create failed\n");
        proc_remove(file);
        proc_remove(dir);
        return -ENOMEM;
    }
    return 0;
}

static void __exit seqfile_exit(void) {
    printk("+ seqfile: exit\n");
    kfree(pidarr);
    proc_remove(sym);
    proc_remove(file);
    proc_remove(dir);
}

module_init(seqfile_init);
module_exit(seqfile_exit);