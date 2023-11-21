#include <linux/module.h>
#include <linux/printk.h>
#include <linux/ktime.h>

#include "rcal_rapl.h"

static struct kobject *rcal_kobj;

// Module interface.

static ssize_t rcal_calibrate_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) 
{
    uint32_t units, start, end, count;

    if (!rcal_cpuid_ok()) {
        return scnprintf(buf, PAGE_SIZE, "{\"error\": \"cpuid failed\"}\n");
    }

    if (!rcal_rdmsr_ok()) {
        return scnprintf(buf, PAGE_SIZE, "{\"error\": \"rdmsr failed\"}\n");
    }

    if (!rcal_rapl_ok()) {
        return scnprintf(buf, PAGE_SIZE, "{\"error\": \"rapl failed\"}\n");
    }

    units = rcal_rapl_units();

    count = 0;
    start = rcal_rapl_sync();
    end = rcal_rapl_sync_c(&count);

    return scnprintf(buf, PAGE_SIZE, "{\"units\": %u, \"start\": %u, \"end\": %u, \"count\": %u}\n", units, start, end, count);
}

static struct kobj_attribute rcal_calibrate_attribute = __ATTR_RO(rcal_calibrate);

static ssize_t rcal_time_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) 
{
    uint64_t start, end;

    if (!rcal_cpuid_ok()) {
        return scnprintf(buf, PAGE_SIZE, "{\"error\": \"cpuid failed\"}\n");
    }

    if (!rcal_rdmsr_ok()) {
        return scnprintf(buf, PAGE_SIZE, "{\"error\": \"rdmsr failed\"}\n");
    }

    if (!rcal_rapl_ok()) {
        return scnprintf(buf, PAGE_SIZE, "{\"error\": \"rapl failed\"}\n");
    }

    rcal_rapl_sync();
    start = ktime_get_ns();
    rcal_rapl_sync();
    end = ktime_get_ns();

    return scnprintf(buf, PAGE_SIZE, "{\"start\": %llu, \"end\": %llu}\n", start, end);
}

static struct kobj_attribute rcal_time_attribute = __ATTR_RO(rcal_time);

int init_module(void) 
{
    int err = 0;

    pr_info("rcal: init_module\n");

    rcal_kobj = kobject_create_and_add("rcal", kernel_kobj);
    if (rcal_kobj == NULL)
        return -ENOMEM;
    
    err = sysfs_create_file(rcal_kobj, &rcal_calibrate_attribute.attr);
    if (err) {
        pr_info("failed to create the rcal_calibrate file in /sys/kernel/rcal\n");
    }

    err = sysfs_create_file(rcal_kobj, &rcal_time_attribute.attr);
    if (err) {
        pr_info("failed to create the rcal_time file in /sys/kernel/rcal\n");
    }

    return 0;
}

void cleanup_module(void) 
{
    pr_info("rcal: cleanup_module\n");
    kobject_put(rcal_kobj);
}

MODULE_LICENSE("GPL");