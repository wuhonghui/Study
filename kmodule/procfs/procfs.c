#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

struct proc_dir_entry *hello = NULL;

#define MAX_PROC_BUFFER_SIZE 30
static char proc_buffer[MAX_PROC_BUFFER_SIZE];
size_t proc_buffer_size = 0;
int proc_hello_read(char *buffer, char **buffer_location, 
           off_t offset, int buffer_length, int *eof, void *data)
{
    int ret = 0;
    if (offset > 0) {
        ret = 0;
    } else {
        memcpy(buffer, proc_buffer, proc_buffer_size);
        ret = proc_buffer_size;
    }
    return ret;
}

int proc_hello_write(struct file *filp, const char *buffer, 
    unsigned long count, void *data)
{
    proc_buffer_size = (count > MAX_PROC_BUFFER_SIZE? MAX_PROC_BUFFER_SIZE : count);
    if (copy_from_user(proc_buffer, buffer, proc_buffer_size)) {
        return -EFAULT;
    }
    return proc_buffer_size;
}

static int __init procfs_init(void)
{
    printk(KERN_INFO"procfs init\n");

    hello = create_proc_entry("hello", 0644, NULL);
    if (!hello) {
        remove_proc_entry("hello", NULL);
        printk(KERN_ERR"failed to create proc entry hello\n");
        return -ENOMEM;
    }

    hello->read_proc = proc_hello_read;
    hello->write_proc = proc_hello_write;
    hello->mode = S_IFREG | S_IRUGO;
    hello->uid = 0;
    hello->gid = 0;
    hello->size = 30;

    return 0;
}

static void __exit procfs_exit(void)
{
    remove_proc_entry("hello", NULL);
    printk(KERN_INFO"procfs exit\n");
}

module_init(procfs_init);
module_exit(procfs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module procfs");
MODULE_AUTHOR("Andel");
