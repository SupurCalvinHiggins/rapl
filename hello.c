#include <linux/module.h>
#include <linux/printk.h>

// https://sysprog21.github.io/lkmpg/#building-modules-for-a-precompiled-kernel
// https://vincent.bernat.ch/en/blog/2017-linux-kernel-microbenchmark

static struct kobject *khello_kobj;

static ssize_t khello_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) 
{
    int ret;
    asm volatile (
        "movl $10, %0"
        : "=r" (ret)
        :
        :
    );
    return scnprintf(buf, PAGE_SIZE, "%#010x\n", ret);
    // return scnprintf(buf, PAGE_SIZE, "Hello world!\n");
}

static struct kobj_attribute khello_attribute = __ATTR_RO(khello);

int init_module(void) 
{
    int err = 0;

    pr_info("khello: init_module\n");

    khello_kobj = kobject_create_and_add("khello", kernel_kobj);
    if (khello_kobj == NULL)
        return -ENOMEM;
    
    err = sysfs_create_file(khello_kobj, &khello_attribute.attr);
    if (err) {
        pr_info("failed to create the khello file in /sys/kernel/khello\n");
    }

    return 0;
}

void cleanup_module(void) 
{
    pr_info("khello: cleanup_module\n");
    kobject_put(khello_kobj);
}

MODULE_LICENSE("GPL");