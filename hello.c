#include <linux/module.h>
#include <linux/printk.h>

static struct kobject *hkello_kobj;

static ssize_t khello_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) 
{
    return scnprintf(buf, PAGE_SIZE, "Hello world!");
}

static struct kobj_attribute khello_attribute = __ATTR_RO(khello);

int init_module(void) 
{
    int err = 0;

    pr_info("khello: init_module\n");

    hkello_kobj = kobject_create_and_add("khello", kernel_kobj);
    if (hkello_kobj == NULL)
        return -ENOMEM;
    
    err = sysfs_create_file(hkello_kobj, &khello_attribute.attr);
    if (err) {
        pr_info("failed to create the khello file in /sys/kernel/khello\n");
    }

    return 0;
}

void cleanup_module(void) 
{
    pr_info("khello: cleanup_module\n");
}

MODULE_LICENSE("GPL");