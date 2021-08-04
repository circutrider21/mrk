#pragma once

#include <cstdint>

extern bool mmu_features[6];

#define PAGE_DIR_COUNT 512
#define IA32_PKS 0x6E1

enum class cache_type {
    normal,
    nocache,
    write_through,
    write_back,
    write_combining
};

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITEABLE (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_NX (1 << 3)
#define PAGE_GLOBAL (1 << 4)

namespace mm::vmm {
void init();
void check_and_init(bool bsp);

struct __attribute__((packed)) PML4 {
    uint64_t entries[PAGE_DIR_COUNT];
};
static_assert(sizeof(PML4) == 4096);

struct __attribute__((packed)) PDPT {
    uint64_t entries[PAGE_DIR_COUNT];
};
static_assert(sizeof(PDPT) == 4096);

struct __attribute__((packed)) PD {
    uint64_t entries[PAGE_DIR_COUNT];
};
static_assert(sizeof(PD) == 4096);

struct __attribute__((packed)) PT {
    uint64_t entries[PAGE_DIR_COUNT];
};

constexpr uint64_t raw_page_present = 0;
constexpr uint64_t raw_page_writeable = 1;
constexpr uint64_t raw_page_user = 2;
constexpr uint64_t raw_page_write_through = 3;
constexpr uint64_t raw_page_cache_disable = 4;
constexpr uint64_t raw_page_pat = 7;
constexpr uint64_t raw_page_global = 8;
constexpr uint64_t raw_page_no_execute = 63;

class aspace {
public:
    void init();
    void clean();
    void load();

    uint64_t get_entry();
    uint64_t virt2phys(uint64_t virt);

    void clone(aspace* new_space);

    bool map(uint64_t phys,
        uint64_t virt,
        uint64_t flags,
        cache_type cache = cache_type::normal,
        int pku = -1);
    bool set_protection(uint64_t virt,
        uint64_t flags,
        cache_type cache = cache_type::normal);
    void unmap(uint64_t virt);

    uint64_t* get_root() { return (uint64_t*)info; }

private:
    PML4* info;
    uint64_t pid;
};

static inline void set_guard()
{
    // Clear AC bit in RFLAGS register.
    if (mmu_features[2])
        asm volatile("clac");
}

static inline void lift_guard(void)
{
    // Set AC bit in RFLAGS register.
    if (mmu_features[2])
        asm volatile("stac");
}

aspace* kernel_space();
}

namespace mm::pkey {
uint8_t alloc(bool supervisor);
void set_perm(uint8_t key,
    bool supervisor,
    bool access_disable,
    bool write_disable);
void free(uint8_t key, bool supervisor);
}

namespace mm::pcid {
uint64_t alloc();
void apply(uint64_t cr3, uint64_t pcid);
void free(uint64_t id);
}
