#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

static struct platform_device *my_device;

static int __init my_device_init(void)
{
    printk(KERN_INFO"my_device init\n");
    my_device = platform_device_alloc("my_dev", -1);
    if (!my_device) {
        printk(KERN_INFO"alloc platform device failed\n");
        return -1;
    }

    if (platform_device_add(my_device)) {
        printk(KERN_INFO"add platform device failed\n");
        return -1;
    }
    return 0;
}

static void __exit my_device_exit(void)
{
    printk(KERN_INFO"my_device exit\n");
    platform_device_unregister(my_device);
}

module_init(my_device_init);
module_exit(my_device_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module my device");
MODULE_AUTHOR("Andel");
