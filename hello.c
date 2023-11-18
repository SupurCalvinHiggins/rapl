#include <linux/module.h>
#include <linux/printk.h>

// https://sysprog21.github.io/lkmpg/#building-modules-for-a-precompiled-kernel
// https://vincent.bernat.ch/en/blog/2017-linux-kernel-microbenchmark

static struct kobject *khello_kobj;

// https://www.kernel.org/doc/Documentation/hwmon/fam15h_power
//   MaxCpuSwPwrAcc MSR C001007b
//   CpuSwPwrAcc MSR C001007a

static inline void cpuid(uint32_t fn, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    asm volatile (
        "cpuid"
        : "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d)
        : "a" (fn)
    );
}

static inline void rdmsr(uint32_t msr, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    asm volatile (
        "rdmsr"
        : "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d)
        : "c" (msr)
    );
}

static inline bool cpuid_ok(void) {
    return true;
}

static inline bool rdmsr_ok(void) {
    // Reading MSRs is ok if CPUID.01H:EDX[5] = 1 as per
    //      1. https://www.felixcloutier.com/x86/rdmsr
    const uint32_t fn = 0x00000001;
    const uint32_t mask = 1 << 5;
    uint32_t unused, d;
    cpuid(fn, &unused, &unused, &unused, &d);
    return d & mask;
}

// AMD

// 15h and 16h
static inline bool amd_energy_ok(void) {
    // Reading energy MSRs is ok if CPUID.80000007H:EDX[12] = 1 as per
    //      1. AMD64 Architecture Progammer's Manual, Volume 2, Section 17.5 
    const uint32_t fn = 0x80000007;
    const uint32_t mask = 1 << 12;
    uint32_t unused, d;
    cpuid(fn, &unused, &unused, &unused, &d);
    return d & mask;  
}

static inline uint32_t amd_energy_acc(void) {
    // CpuSwPwrAcc: https://www.kernel.org/doc/Documentation/hwmon/fam15h_power
    uint32_t unused, a, d;
    rdmsr(0xc001007a, &a, &unused, &unused, &d);
    return ((uint64_t)d << 32) | a;
}

static inline uint32_t amd_energy_max(void) {
    // MaxCpuSwPwrAcc: https://www.kernel.org/doc/Documentation/hwmon/fam15h_power
    uint32_t unused, a, d;
    rdmsr(0xc001007b, &a, &unused, &unused, &d);
    return ((uint64_t)d << 32) | a;
}

// 17h

// same as intel but different locations
// c001_0299 rapl_power_unit, file:///home/calvin/snap/firefox/common/Downloads/AMD%20-%20Preliminary%20Processor%20Programming%20Reference%20(PPR)%20for%20AMD%20Family%2017h%20Models%2000h-0Fh%20Processors%20-%20Rev%201.14%20-%20April%2015th,%202017%20(54945).pdf
// c001_029b pkg_energy_stat

// Intel

static inline uint32_t intel_energy_units(void) {
    const uint32_t msr = 0x00000606; // MSR_RAPL_POWER_UNIT. https://github.com/torvalds/linux/blob/b8f1fa2419c19c81bc386a6b350879ba54a573e1/arch/x86/include/asm/msr-index.h#L392, https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-3b-part-2-manual.pdf
    const uint32_t offset = 8;
    const uint32_t mask = 0xF << offset;
    uint32_t unused, a;
    rdmsr(msr, &a, &unused, &unused, &unused);
    return (a & mask) >> shift;
}

static inline uint32_t intel_energy(void) {
    const uint32_t msr = 0x00000611; // MSR_PKG_ENERGY_STATUS. https://github.com/torvalds/linux/blob/b8f1fa2419c19c81bc386a6b350879ba54a573e1/arch/x86/include/asm/msr-index.h#L392
    uint32_t unused, a;
    rdmsr(msr, &a, &unused, &unused, &unused);
    return a;
}

// Generic

static inline uint32_t energy_sync(void) {
    uint32_t start, end; 
    start = intel_energy();
    while ((end = intel_energy()) == start);
    return end;
}

static ssize_t khello_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) 
{
    return scnprintf(buf, PAGE_SIZE, "%#010llx\n", msr_ok());
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