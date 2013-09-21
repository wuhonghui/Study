#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

static int my_device_probe(struct platform_device *pdev)
{
    printk(KERN_INFO"Probe my device\n");
    return 0;
}

static int my_device_remove(struct platform_device *pdev)
{
    printk(KERN_INFO"Remove my device\n");
    return 0;
}

static struct platform_driver my_driver = {
    .probe = my_device_probe,
    .remove = my_device_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "my_dev",
    },
};

static int __init my_driver_init(void)
{
    printk(KERN_INFO"my driver init\n");
    if (platform_driver_register(&my_driver)) {
        printk(KERN_INFO"my driver register failed\n");
        return -1;
    }
    return 0;
}

static void __exit my_driver_exit(void)
{
    printk(KERN_INFO"my driver exit\n");
    platform_driver_unregister(&my_driver);
}

module_init(my_driver_init);
module_exit(my_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module my device driver");
MODULE_AUTHOR("Andel");
