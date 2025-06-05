#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <asm/io.h>


#define IRQ_NUM 1
#define SCANCODE_PORT 0x60
#define STATUS_PORT 0x64
#define FILENAME "my_wq"
#define SLEEP_TIME 10000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Impervguin");
MODULE_DESCRIPTION("Module with keyboard interrupt tasklet");


static struct proc_dir_entry *file;

static int counter = 0;
static char *last_symbol = "No symbol";
static ktime_t last_kwork_time = 0;

static struct workqueue_struct *my_wq;

static struct work_struct sleep_work;


char * symbs[84] =  {
    " ", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
    "-", "+", "Backspace", 
    "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]",
    "Enter", "Ctrl",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "\"", "'",
    "Left Shift", "|", 
    "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "Right Shift", 
    "*", "Alt", "Space", "CapsLock", 
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
    "NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
    " ", "Right", "+",
    "End", "Down", "Page-Down", "Insert", "Delete"
};

struct my_work {
    struct work_struct work;
    char code;
    ktime_t start_time;
};

static void workqueue_func(struct work_struct *work) {
    struct my_work *my_work = container_of(work, struct my_work, work);
    char code = my_work->code;

    if (code != 28 && code != 80 && code != 77 && code != 75 && code != 72 && code < 84) {
        last_symbol = symbs[code];
        printk(KERN_INFO "+ workqueue: code=%d, symbol=%s\n", code, last_symbol);
        counter++;
        my_work->code = 0;
        ktime_t now = ktime_get();
        last_kwork_time = now - my_work->start_time;
    }

    kfree(my_work);
}

static void workqueue_sleep(struct work_struct *work) {
    printk(KERN_INFO "+ workqueue: sleep\n");
    msleep(SLEEP_TIME);
    printk(KERN_INFO "+ workqueue: wake up\n");
}

static irqreturn_t my_handler(int irq, void *dev_id) {
    if (IRQ_NUM == irq) {
        char scancode = inb(SCANCODE_PORT);
        char status = inb(STATUS_PORT);
        struct my_work *my_work = kmalloc(sizeof(struct my_work), GFP_KERNEL);
        if (!my_work) {
            printk(KERN_ERR "+ workqueue: kmalloc failed\n");
            return IRQ_HANDLED;
        }
        my_work->code = scancode;
        my_work->start_time = ktime_get();
        INIT_WORK(&my_work->work, workqueue_func);
        queue_work(my_wq, &my_work->work);
        queue_work(my_wq, &sleep_work);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

static int my_single_show(struct seq_file *s, void *v) {
    seq_printf(s, "Counter: %d\n", counter);
    seq_printf(s, "Symbol: %s\n", last_symbol);
    seq_printf(s, "Workqueue time: %llu ns\n", last_kwork_time);
    return 0;
}

static int single_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, my_single_show, NULL);
}

static const struct proc_ops single_proc_fops = {
    .proc_open = single_proc_open,
    .proc_release = single_release,
    .proc_read = seq_read,
};


static int __init my_init(void) {
    printk(KERN_INFO "+ workqueue: init\n");

    my_wq = alloc_workqueue("my_wq", 0, 0);
    if (!my_wq) {
        printk(KERN_ERR "+ workqueue: alloc_workqueue failed\n");
        return -ENOMEM;
    }
    INIT_WORK(&sleep_work, workqueue_sleep);

    int err = request_irq(IRQ_NUM, my_handler, IRQF_SHARED, "my_wq_handler", &my_handler);
    if (err) {
        printk(KERN_ERR "+ workqueue: request_irq failed\n");
        return -ENOMEM;
    }
    printk(KERN_INFO "+ workqueue: irq requested\n");
    file = proc_create(FILENAME, 0666, NULL, &single_proc_fops);
    if (file == NULL) {
        printk(KERN_ERR "+ workqueue: proc_create failed\n");
        free_irq(IRQ_NUM, NULL);
        return -ENOMEM;
    }
    printk(KERN_INFO "+ workqueue: proc file created\n");
    return 0;
}

static void __exit my_exit(void) {
    printk(KERN_INFO "+ workqueue: exit\n");
    // free_irq(IRQ_NUM, NULL);
    // restore default handler
    int err = request_irq(IRQ_NUM, NULL, 0, NULL, NULL);
    if (err) {
        printk(KERN_ERR "+ workqueue: request_irq failed\n");
        return;
    }
    printk(KERN_INFO "+ workqueue: irq restored\n");
    flush_workqueue(my_wq);
    destroy_workqueue(my_wq);
    printk(KERN_INFO "+ workqueue: workqueue destroyed\n");
}

module_init(my_init);
module_exit(my_exit);