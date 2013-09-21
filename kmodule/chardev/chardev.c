#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

static int major;
static int isopened = 0;

#define BUFFER_SIZE 256
static char chardev_buffer[BUFFER_SIZE + 1];
static char *p_buffer = 0;

static ssize_t chardev_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
    ssize_t nread = 0;

    if (*p_buffer == '\0')
        return 0;

    while(*p_buffer != '\0' && length > 0) {
        put_user(*p_buffer, buffer);

        buffer++;
        p_buffer++;
        length--;
        nread++;
    }

    return nread;
}

static ssize_t chardev_write(struct file *filp, 
                            const char __user *buffer, 
                            size_t length, loff_t *offset)
{
    ssize_t n_nowrite = 0;
    
    if (length > BUFFER_SIZE) {
        return -EFBIG;
    }

    n_nowrite = copy_from_user(chardev_buffer, buffer, length);

    return length - n_nowrite;
}

static int chardev_open(struct inode *inode, struct file *filp)
{
    if (isopened)
        return -EBUSY;

    isopened++;
    p_buffer = chardev_buffer;

    try_module_get(THIS_MODULE);

    return 0;
}

static int chardev_release(struct inode *inode, struct file *filp)
{
    module_put(THIS_MODULE);

    isopened--;

    return 0;
}

static struct file_operations chardev_fops = {
    .read = chardev_read,
    .write = chardev_write,
    .open = chardev_open,
    .release = chardev_release,
};

static int __init chardev_init(void)
{
    printk(KERN_INFO"chardev init\n");

    major = register_chrdev(0, "chardev", &chardev_fops);
    if (major < 0) {
        printk(KERN_ERR"Register char device chardev failed, %d\n", major);
        return major;
    }

    printk(KERN_INFO"\tmajor : %d\n", major);
    printk(KERN_INFO"\trun mknod /dev/%s c %d 0\n", "chardev", major);
    printk(KERN_INFO"\ttry cat and echo to device, remove device file when done.\n");

    return 0;
}

static void __exit chardev_exit(void)
{
    unregister_chrdev(major, "chardev");
    printk(KERN_INFO"chardev exit\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module chardev");
MODULE_AUTHOR("Andel");
