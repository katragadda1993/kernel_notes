#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

static int __init lesson03_hello_init(void)
{
    pr_info("lesson03_hello: loaded\n");
    return 0;
}

static void __exit lesson03_hello_exit(void)
{
    pr_info("lesson03_hello: unloaded\n");
}

module_init(lesson03_hello_init);
module_exit(lesson03_hello_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linux kernel masterclass Kbuild example");
