#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/cred.h>
#include <linux/syscalls.h>

/* sys call table */
static unsigned long **sys_call_table;

/* uid */
static int uid = 0;
module_param(uid, int, 0644);

asmlinkage int (*origin_syscall)(const char *, int, int);

static unsigned long **find_sys_call_table(void)
{
    unsigned long int offset = PAGE_OFFSET;
    unsigned long **sct;

    while (offset < ULLONG_MAX) {
        sct = (unsigned long **)offset;
        if (sct[__NR_close] == (unsigned long *)sys_close)
            return sct;

        offset += sizeof(void *);
    }

    return NULL;
}

static void enable_page_protection(void)
{
    unsigned long value = read_cr0();

    if ((value & 0x00010000))
        return;

    write_cr0(value | 0x10000);
}
static void disable_page_protection(void)
{
    unsigned long value = read_cr0();
    if (!(value & 0x00010000))
        return;

    write_cr0(value & ~0x10000);
}

asmlinkage int my_sys_open(const char *filename, int flags, int mode)
{
    if (uid == current->cred->uid) {
        printk(KERN_NOTICE"uid %d open file %s\n", uid, filename);
    }
    return origin_syscall(filename, flags, mode);
}

static int __init syscall_injection_init(void)
{
    printk(KERN_INFO"syscall_injection init\n");
    sys_call_table = find_sys_call_table();
    disable_page_protection();
    if (sys_call_table) {
        origin_syscall = (void *)sys_call_table[__NR_open];
        sys_call_table[__NR_open] = (unsigned long *)my_sys_open;
    }
    enable_page_protection();
    return 0;
}

static void __exit syscall_injection_exit(void)
{
    printk(KERN_INFO"syscall_injection exit\n");

    disable_page_protection();
    if (sys_call_table) {
        sys_call_table[__NR_open] = (unsigned long *)origin_syscall;
    }
    enable_page_protection();
}

module_init(syscall_injection_init);
module_exit(syscall_injection_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module syscall_injection");
MODULE_AUTHOR("Andel");
