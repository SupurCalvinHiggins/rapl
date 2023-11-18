#include <linux/module.h>
#include <linux/printk.h>

// https://sysprog21.github.io/lkmpg/#building-modules-for-a-precompiled-kernel
// https://vincent.bernat.ch/en/blog/2017-linux-kernel-microbenchmark

static struct kobject *khello_kobj;

// https://www.kernel.org/doc/Documentation/hwmon/fam15h_power
//   MaxCpuSwPwrAcc MSR C001007b
//   CpuSwPwrAcc MSR C001007a

static inline uint64_t read_msr(uint32_t msr) {
    uint32_t low, high;
    asm volatile (
        "rdmsr"
        : "=a" (low), "=d" (high)
        : "c" (msr)
    );
    return ((uint64_t)high << 32) | low;
}

static inline bool msr_ok(void) {
    uint32_t reg[4];
    asm volatile (
        "cpuid"
        : "=a" (reg[0]), "=b" (reg[1]), "=c" (reg[2]), "=d" (reg[3])
        : "a" (0x1) // https://www.felixcloutier.com/x86/rdmsr
    );
    return reg[4] & (1 << 5); 
}

static inline bool has_energy_counter(void) {
    uint32_t reg[4];
    asm volatile (
        "cpuid"
        : "=a" (reg[0]), "=b" (reg[1]), "=c" (reg[2]), "=d" (reg[3])
        : "a" (0x80000007) // Section 17.5 AMD64 arch programmer's manual volume 2.
                           // bit 12 of EDX is flag indicating energy facilities
    );
    return reg[4] & (1 << 12); 
}

static ssize_t khello_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) 
{
    const uint32_t MaxCpuSwPwrAcc = 0xc001007b; // maximum value of CpuSwPwrAcc
    const uint32_t CpuSwPwrAcc = 0xc001007a;    // acculumated power usage
    uint64_t ret1, ret2;
    
    if (!msr_ok()) {
        return scnprintf(buf, PAGE_SIZE, "msr not ok");
    }
    
    ret1 = read_msr(MaxCpuSwPwrAcc);
    ret2 = read_msr(CpuSwPwrAcc);

    // uint64_t ret = has_energy_counter();
    return scnprintf(buf, PAGE_SIZE, "%#010llx %#010llx\n", ret1, ret2);
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