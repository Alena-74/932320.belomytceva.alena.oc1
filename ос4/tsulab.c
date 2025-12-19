#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/string.h>

#define procfs_name "tsulab"
static struct proc_dir_entry* our_proc_file = NULL;

static struct timespec64 doomsday = {
    .tv_sec = 0,
    .tv_nsec = 0
};

static ssize_t profile_read(struct file* file_pointer, char __user* buffer,
    size_t buffer_length, loff_t* offset)
{
    struct timespec64 now;
    s64 minutes_ago;
    char s[256];
    size_t len = 0;

    ktime_get_real_ts64(&now);

    minutes_ago = (now.tv_sec - doomsday.tv_sec) / 60;
    len = snprintf(s, sizeof(s),
        "Tomsk State University\nDoomsday (Terminator 2): August 29, 1997, 02:14 AM\n"
        "Doomsday was %lld minutes ago\n", minutes_ago);
    
    if (*offset >= len)
        return 0;

    if (buffer_length < len)
        return -EINVAL;

    if (copy_to_user(buffer, s, len))
    {
        return -EFAULT;
    }

    *offset += len;
    pr_info("profile read %s\n", file_pointer->f_path.dentry->d_name.name);
    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = profile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = profile_read,
};
#endif

static void set_doomsday_time(void)
{
    struct tm tm_time = {
        .tm_year = 1997 - 1900,  
        .tm_mon = 7,             
        .tm_mday = 29,           
        .tm_hour = 2,           
        .tm_min = 14,            
        .tm_sec = 0,             
        .tm_isdst = 0          
    };

    doomsday.tv_sec = mktime64(&tm_time);
    doomsday.tv_nsec = 0;
}

static int __init procfs1_init(void)
{
    set_doomsday_time();

    our_proc_file = proc_create(procfs_name, 0644, NULL, &proc_file_fops);
    if (our_proc_file == NULL)
    {
        pr_err("Error: Could not initialize /proc/%s\n", procfs_name);
        return -ENOMEM;
    }
    pr_info("Welcome to the Tomsk State University\n");
    pr_info("/proc/%s created\n", procfs_name);
    return 0;
}

static void __exit procfs1_exit(void)
{
    proc_remove(our_proc_file);
    pr_info("Tomsk State University forever!\n");
    pr_info("/proc/%s removed\n", procfs_name);
}

module_init(procfs1_init);
module_exit(procfs1_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Terminator 2 Doomsday Calculator");