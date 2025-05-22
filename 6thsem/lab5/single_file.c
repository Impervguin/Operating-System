#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>


#define DIRNAME "singlefile"
#define FILENAME "singlefile_src"
#define SYMNAME "singlefile_sym"

#define BUF_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Impervguin");
MODULE_DESCRIPTION("Module for getting task info by pid via seq_file with single open");

static struct proc_dir_entry *dir;
static struct proc_dir_entry *file;
static struct proc_dir_entry *sym;

static char singlefile_buf[BUF_SIZE];
static int *pidarr;
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

static int my_single_show(struct seq_file *s, void *v) {
    if (pidarr == NULL) {
        printk("+ single_file: show, seq=%p, pidarr is NULL\n", s);
    } else {
        printk("+ single_file: show, seq=%p, pidarr=%p\n", s, v);
    }

    for (int i = 0; i < pidarr_count; i++) {
        int nr = pidarr[i];

        struct task_struct *task;
        struct pid *pid = find_vpid(nr);
        task = pid_task(pid, PIDTYPE_PID);
        if (task == NULL) {
            seq_printf(s, "No such pid %d\n", nr);
            return 0;
        }

    char policy[PARSE_SIZE];
    parse_policy(policy, task->policy);
    char exit_state[PARSE_SIZE];
    parse_exit_state(exit_state, task->exit_state);
    char flags[BUF_SIZE];
    print_task_flags(task->flags, flags, BUF_SIZE);
    snprintf(singlefile_buf, BUF_SIZE, 
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

    seq_printf(s, "%s\n", singlefile_buf);

    }

    return 0;
}

static int single_proc_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "+ single_file: open\n");
    return single_open(file, my_single_show, pidarr);
}


static ssize_t single_proc_read(struct file *f, char __user *c, size_t s, loff_t *off)
{
    printk(KERN_INFO "+ single_file: read\n");
    return seq_read(f, c, s, off);
}

static ssize_t single_proc_write(struct file *f, const char __user *c, size_t count, loff_t *f_pos)
{
    printk("+ single_file: write, f_pos=%lld, count=%zu\n", *f_pos, count);
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

static int single_proc_release(struct inode *inode, struct file *file)
{
    printk("+ single_file: release\n");
    return single_release(inode, file);
}

static const struct proc_ops single_proc_fops = {
    .proc_open = single_proc_open,
    .proc_release = single_proc_release,
    .proc_read = single_proc_read,
    .proc_write = single_proc_write,
};


static int __init singlefile_init(void) {
    printk(KERN_INFO "+ single_file: init\n");
    pidarr = kmalloc(sizeof(int), GFP_KERNEL);
    if (!pidarr) {
        printk(KERN_ERR "singlefile: kmalloc failed\n");
        return -ENOMEM;
    }
    *pidarr = -1;
    printk(KERN_INFO "pidarr=%p\n", pidarr);
    dir = proc_mkdir(DIRNAME, NULL);
    if (dir == NULL) {
        kfree(pidarr);
        printk(KERN_ERR "singlefile: proc_mkdir failed\n");
        return -ENOMEM;
    }
    file = proc_create(FILENAME, 0666, dir, &single_proc_fops);
    if (file == NULL) {
        printk(KERN_ERR "singlefile: proc_create failed\n");
        kfree(pidarr);
        proc_remove(file);
        return -ENOMEM;
    }
    sym = proc_symlink(SYMNAME, NULL, DIRNAME "/" FILENAME);
    if (sym == NULL) {
        printk(KERN_ERR "singlefile: proc_create failed\n");
        kfree(pidarr);
        proc_remove(file);
        proc_remove(dir);
        return -ENOMEM;
    }
    return 0;
}

static void __exit singlefile_exit(void) {
    printk("+ single_file: exit\n");
    kfree(pidarr);
    proc_remove(sym);
    proc_remove(file);
    proc_remove(dir);
}

module_init(singlefile_init);
module_exit(singlefile_exit);   

