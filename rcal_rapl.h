#ifndef H_RCAL_RAPL
#define H_RCAL_RAPL

#include <linux/types.h>

// Platform specific configuration.

#define RCAL_AMD    0
#define RCAL_INTEL  1
#define RCAL_ARCH   RCAL_AMD

#if RCAL_ARCH == RCAL_AMD

#define RCAL_RAPL_POWER_UNIT        0xc0010299
#define RCAL_RAPL_PKG_ENERGY_STATUS 0xc001029b

#elif RCAL_ARCH == RCAL_INTEL

#define RCAL_RAPL_POWER_UNIT        0x00000606
#define RCAL_RAPL_PKG_ENERGY_STATUS 0x00000611

#endif

// Assembly instructions.

static inline void rcal_cpuid(uint32_t fn, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    asm volatile (
        "cpuid"
        : "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d)
        : "a" (fn)
    );
}

static inline void rcal_rdmsr(uint32_t msr, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    asm volatile (
        "rdmsr"
        : "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d)
        : "c" (msr)
    );
}

// Hardware checks.

static inline bool rcal_cpuid_ok(void) {
    return true;
}

static inline bool rcal_rdmsr_ok(void) {
    const uint32_t fn = 0x00000001;
    const uint32_t mask = 1 << 5;
    uint32_t unused, d;
    rcal_cpuid(fn, &unused, &unused, &unused, &d);
    return d & mask;
}

static inline bool rcal_rapl_ok(void) {
    const uint32_t fn = 0x80000007;
    const uint32_t mask = 1 << 14;
    uint32_t unused, d;
    rcal_cpuid(fn, &unused, &unused, &unused, &d);
    return d & mask;
}

// RAPL interface.

static inline uint32_t rcal_rapl_units(void) {
    const uint32_t mask = 0x00001F00;
    uint32_t unused, a;
    rcal_rdmsr(RCAL_RAPL_POWER_UNIT, &a, &unused, &unused, &unused);
    return (a & mask) >> 8;
}

static inline uint32_t rcal_rapl_energy(void) {
    uint32_t unused, a;
    rcal_rdmsr(RCAL_RAPL_PKG_ENERGY_STATUS, &a, &unused, &unused, &unused);
    return a;
}

static inline uint32_t rcal_rapl_sync(void) {
    uint32_t start, end; 
    start = rcal_rapl_energy();
    while ((end = rcal_rapl_energy()) == start);
    return end;
}

static inline uint32_t rcal_rapl_sync_c(uint32_t* count) {
    uint32_t start, end; 
    start = rcal_rapl_energy();
    while ((end = rcal_rapl_energy()) == start) (*count)++;
    return end;
}

#endif