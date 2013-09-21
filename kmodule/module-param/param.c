/*
 * make
 *
 * insmod param.ko var_int=2 var_int_perm_600=3 var_int_perm_644=4 var_short=5 var_int_array=6,7 var_string="string" var_char_pointer="pointer"
 *
 * dmesg|tail -n 20
 *
 * ll /sys/module/param/parameters/ -l
 *
 * rmmod param.ko
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int var_int = 0;
module_param(var_int, int, 0);

static int var_int_perm_600 = 0;
module_param(var_int_perm_600, int, S_IRUSR | S_IWUSR);

static int var_int_perm_644 = 0;
module_param(var_int_perm_644, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

static short var_short = 0;
module_param(var_short, short, 0);

static int var_int_array_size = 0;
static int var_int_array[3] = {0, 0, 0};
module_param_array(var_int_array, int, &var_int_array_size, 0);

static char var_string[10] = "\0";
module_param_string(var_string, var_string, 10, 0);

static char *var_char_pointer = NULL;
module_param(var_char_pointer, charp, 0);
MODULE_PARM_DESC(var_char_pointer, "a char pointer module param");

static int __init hello_init(void)
{
    int i = 0;
    printk(KERN_INFO"hello init\n");
    printk(KERN_INFO"var_int : %d\n", var_int);
    printk(KERN_INFO"var_int_perm_600 : %d\n", var_int_perm_600);
    printk(KERN_INFO"var_int_perm_644 : %d\n", var_int_perm_644);
    printk(KERN_INFO"var_short : %d\n", var_short);

    printk(KERN_INFO"var_int_array_size : %d\n", var_int_array_size);
    for (i = 0; i < sizeof(var_int_array)/sizeof(int); ++i) {
        printk(KERN_INFO"var_int_array[%d] : %d\n", i, var_int_array[i]);
    }

    for (i = 0; i < sizeof(var_string)/sizeof(char); ++i) {
        printk(KERN_INFO"var_string[%d] : '%c'\n", i, var_string[i]);
    }

    printk(KERN_INFO"var_char_pointer : %s\n", var_char_pointer);

    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO"hello exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module hello");
MODULE_AUTHOR("Andel");
