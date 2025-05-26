# include <linux/module.h>
# include <linux/kernel.h>
# include <linux/init.h>
# include <linux/fs.h>
# include <linux/time.h>
# include <linux/mnt_idmapping.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Impervguin");
MODULE_DESCRIPTION("Module with simple vfs");

#define MYFS_MAGIC_NUMBER 0x42424242
#define MYFS_SLAB_NAME "myfs_slab"
#define MYFS_CACHE_SIZE 1

struct myfs_inode
{
    int i_mode;
    unsigned long i_ino;
};

struct myfs_cache_entry {
    struct myfs_inode inode;
    struct myfs_inode children[127];
};

struct myfs_sb_info {
    struct myfs_cache_entry **cache;
    int num_cache;
};


static struct kmem_cache *myfs_slab;

static struct super_operations const myfs_super_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};


static struct inode * myfs_make_inode(struct super_block * sb, int mode)
{
    struct inode * ret = new_inode(sb);
    if (ret)
    {
        inode_init_owner(&nop_mnt_idmap, ret, NULL, mode);
        ret->i_size = PAGE_SIZE;
        struct timespec64 cur = current_time(ret);
        inode_set_atime_to_ts(ret, cur);
        inode_set_mtime_to_ts(ret, cur);
        inode_set_ctime_to_ts(ret, cur);
        ret->i_ino = 1;
    }
    return ret;
}


static int myfs_fill_sb(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = MYFS_MAGIC_NUMBER;
    sb->s_op = &myfs_super_ops;


    root_inode = myfs_make_inode(sb, S_IFDIR | 0755);
    if (!root_inode) {
        printk(KERN_ERR "+ myfs: myfs_make_inode error\n");
        return -ENOMEM;
    }

    struct timespec64 cur = current_time(root_inode);
    inode_set_atime_to_ts(root_inode, cur);
    inode_set_mtime_to_ts(root_inode, cur);
    inode_set_ctime_to_ts(root_inode, cur);
    root_inode->i_op = &simple_dir_inode_operations;
    root_inode->i_fop = &simple_dir_operations;

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        printk(KERN_ERR "+ myfs: d_make_root error\n");
        iput(root_inode);
        return -ENOMEM;
    }

    printk(KERN_INFO "+ myfs: vfs root created\n");
    return 0;
}

static void kill_myfs(struct super_block *sb) {
    struct myfs_sb_info *sbi = sb->s_fs_info;
    for (int i = 0; i < sbi->num_cache; i++) {
        struct myfs_cache_entry *entry = sbi->cache[i];
        if (entry) {
            kmem_cache_free(myfs_slab, entry);
        }
    }
    kfree(sbi->cache);
    sbi->cache = NULL;
    sbi->num_cache = 0;
    kill_block_super(sb);
}

static struct dentry *myfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    struct dentry *root = mount_bdev(fs_type, flags, dev_name, data, myfs_fill_sb);
    if (IS_ERR(root)) {
        printk(KERN_ERR "+ myfs: mount_bdev failed\n");
        return root;
    }
    struct super_block *sb = root->d_sb;
    struct myfs_sb_info *sbi = kmalloc(sizeof(struct myfs_sb_info), GFP_KERNEL);
    if (!sbi) {
        printk(KERN_ERR "+ myfs: kmalloc failed\n");
        kill_myfs(sb);
        return ERR_PTR(-ENOMEM);
    }
    memset(sbi, 0, sizeof(struct myfs_sb_info));

    printk(KERN_INFO "+ myfs: mount_bdev succeeded\n");

    sbi->num_cache = MYFS_CACHE_SIZE;
    sbi->cache = kcalloc(sbi->num_cache, sizeof(struct myfs_cache_entry*), GFP_KERNEL);
    if (!sbi->cache) {
        printk(KERN_ERR "+ myfs: cache allocation failed\n");
        kill_myfs(sb);
        return ERR_PTR(-ENOMEM);
    }

    for (int i = 0; i < sbi->num_cache; i++) {
        struct myfs_cache_entry *entry = kmem_cache_alloc(myfs_slab, GFP_KERNEL);
        if (!entry) {
            printk(KERN_ERR "+ myfs: slab allocation failed for entry %d\n", i);
            kill_myfs(sb);
            return ERR_PTR(-ENOMEM);
        }

        entry->inode.i_mode = S_IFREG | 0644;
        entry->inode.i_ino = i + 1; 
        sbi->cache[i] = entry;
    }

    sb->s_fs_info = sbi;

    return root;
}


struct file_system_type myfs_type = {
    .owner = THIS_MODULE,
    .name = "myfs",
    .mount = myfs_mount,
    .kill_sb = kill_myfs,
    .fs_flags = FS_REQUIRES_DEV,
};

static int __init myfs_init(void) {
    printk(KERN_INFO "+ myfs: init\n");
    printk(KERN_INFO "+ myfs: sizeof(struct myfs_cache_entry) = %d\n", sizeof(struct myfs_cache_entry));
    printk(KERN_INFO "+ myfs: sizeof(struct myfs_inode) = %d\n", sizeof(struct myfs_inode));
    
    myfs_slab = kmem_cache_create(MYFS_SLAB_NAME, 
        sizeof(struct myfs_cache_entry),
        0, 
        SLAB_HWCACHE_ALIGN | SLAB_PANIC | SLAB_ACCOUNT,
        NULL);
    
    if (!myfs_slab) {
        printk(KERN_ERR "+ myfs: kmem_cache_create failed\n");
        return -ENOMEM;
    }
    printk(KERN_INFO "+ myfs: slab cache created\n");

    int ret = register_filesystem(&myfs_type);
    if (ret) {
        printk(KERN_ERR "+ myfs: register_filesystem failed\n");
        return ret;
    }
    printk(KERN_INFO "+ myfs: register_filesystem succeeded\n");
    return 0;
}

static void __exit myfs_exit(void) {
    printk(KERN_INFO "+ myfs: exit\n");
    int ret = unregister_filesystem(&myfs_type);
    if (ret) {
        printk(KERN_ERR "+ myfs: unregister_filesystem failed\n");
        return;
    }
    printk(KERN_INFO "+ myfs: unregister_filesystem succeeded\n");
}

module_init(myfs_init);
module_exit(myfs_exit);