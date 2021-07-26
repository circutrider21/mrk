#include <internal/builtin.h>
#include <internal/stivale2.h>
#include <mrk/arch.h>
#include <mrk/cpu.h>
#include <mrk/idt.h>
#include <mrk/lock.h>
#include <mrk/log.h>
#include <mrk/pmm.h>
#include <mrk/vmm.h>

#include <cstddef>

#define GETMASK(index, size) ((((size_t)1 << (size)) - 1) << (index))
#define WRITETO(data, index, size, value)             \
    ((data) = (((data) & (~GETMASK((index), (size)))) \
         | (((value) << (index)) & (GETMASK((index), (size))))))

bool mmu_features[6];
mm::vmm::aspace kspace;
static mutex vmm_mutex;

using namespace arch::cpu;
using namespace arch::idt;

// Some page table modification/indexing functions
static constexpr uint64_t
index_pml4(uint64_t address)
{
    return (address >> 39) & 0x1FF;
}

static constexpr uint64_t
index_pdpt(uint64_t address)
{
    return (address >> 30) & 0x1FF;
}

static constexpr uint64_t
index_pd(uint64_t address)
{
    return (address >> 21) & 0x1FF;
}

static constexpr uint64_t
index_pt(uint64_t address)
{
    return (address >> 12) & 0x1FF;
}

static constexpr void
frame_set(uint64_t& entry, uint64_t phys)
{
    entry |= (phys & 0x000FFFFFFFFFF000);
}

static constexpr uint64_t
get_frame(uint64_t entry)
{
    return entry & 0x000FFFFFFFFFF000;
}

static constexpr void
set_flags(uint64_t& entry, uint64_t flags)
{
    entry |= (flags & 0xFFF0000000000FFF);
}

static constexpr uint64_t
get_flags(uint64_t entry)
{
    return (entry & 0xFFF0000000000FFF);
}

static constexpr uint64_t
cache2pat(cache_type types)
{
    uint64_t ret = 0;

    switch (types) {
    case cache_type::normal: // Default is already set
        [[fallthrough]];
    case cache_type::write_back: // Not yet supported
        break;
    case cache_type::write_combining:
        ret |= (1 << mm::vmm::raw_page_write_through);
        break;
    case cache_type::write_through:
        ret |= (1 << mm::vmm::raw_page_cache_disable);
        break;
    case cache_type::nocache:
        ret |= (1 << mm::vmm::raw_page_write_through);
        ret |= (1 << mm::vmm::raw_page_pat);
        break;
    }

    return ret;
}

static mm::vmm::PT* clone_pt(mm::vmm::PT* pt)
{
    mm::vmm::PT* new_pt = (mm::vmm::PT*)((uint64_t)mm::pmm::get(1) + MEM_VIRT_OFFSET);
    _memset((uint64_t)new_pt, 0, 4096);

    for (uint64_t i = 0; i < PAGE_DIR_COUNT; i++) {
        new_pt->entries[i] = pt->entries[i]; // Just copy it over
    }

    return (mm::vmm::PT*)((uint64_t)new_pt - MEM_VIRT_OFFSET);
}

static void clean_pd(mm::vmm::PD* pd)
{
    for (uint64_t loop_index = 0; loop_index < PAGE_DIR_COUNT; loop_index++) {
        uint64_t pt_entry = pd->entries[loop_index];

        if (pt_entry & (1ull << 0)) {
            if (!(pt_entry & (1ull << 7))) { // Not a huge page
                uint64_t pt_addr = (pt_entry & 0x000FFFFFFFFFF000);
                mm::pmm::free((void*)pt_addr, 1);
            }
        }
    }
}

static mm::vmm::PD* clone_pd(mm::vmm::PD* pd)
{
    mm::vmm::PD* new_pd = (mm::vmm::PD*)((uint64_t)mm::pmm::get(1) + MEM_VIRT_OFFSET);
    _memset((uint64_t)new_pd, 0, 4096);

    for (uint64_t i = 0; i < PAGE_DIR_COUNT; i++) {
        uint64_t old_entry = pd->entries[i];

        uint64_t entry_flags = get_flags(old_entry);

        if (entry_flags & (1ull << 0)) {
            if (!(entry_flags & (1ull << 7))) {
                uint64_t pd = (uint64_t)clone_pt((mm::vmm::PT*)get_frame(old_entry) + MEM_VIRT_OFFSET);

                uint64_t new_entry = 0;
                frame_set(new_entry, pd);
                set_flags(new_entry, get_flags(old_entry));

                new_pd->entries[i] = new_entry;
            } else {
                new_pd->entries[i] = old_entry;
            }
        }
    }

    return (mm::vmm::PD*)((uint64_t)new_pd - MEM_VIRT_OFFSET);
}

static void clean_pdpt(mm::vmm::PDPT* pdpt)
{
    for (uint64_t loop_index = 0; loop_index < PAGE_DIR_COUNT; loop_index++) {
        uint64_t pd_entry = pdpt->entries[loop_index];

        if (pd_entry & (1ull << 0)) {
            if (!(pd_entry & (1ull << 7))) {
                uint64_t pd_addr = ((pd_entry & 0x000FFFFFFFFFF000) + MEM_VIRT_OFFSET);
                clean_pd((mm::vmm::PD*)pd_addr);

                mm::pmm::free((void*)((uint64_t)pd_entry & 0x000FFFFFFFFFF000), 1);
            }
        }
    }
}

static mm::vmm::PDPT* clone_pdpt(mm::vmm::PDPT* pdpt)
{
    mm::vmm::PDPT* new_pdpt = (mm::vmm::PDPT*)((uint64_t)mm::pmm::get(1) + MEM_VIRT_OFFSET);
    _memset((uint64_t)new_pdpt, 0, 4096);

    for (uint64_t i = 0; i < PAGE_DIR_COUNT; i++) {
        uint64_t old_entry = pdpt->entries[i];

        uint64_t entry_flags = get_flags(old_entry);

        if (entry_flags & (1ull << 0)) {
            if (!(entry_flags & (1ull << 7))) {
                uint64_t pdpt = (uint64_t)clone_pd((mm::vmm::PD*)get_frame(old_entry) + MEM_VIRT_OFFSET);

                uint64_t new_entry = 0;
                frame_set(new_entry, pdpt);
                set_flags(new_entry, get_flags(old_entry));

                new_pdpt->entries[i] = new_entry;
            } else {
                new_pdpt->entries[i] = old_entry;
            }
        }
    }

    return (mm::vmm::PDPT*)((uint64_t)new_pdpt - MEM_VIRT_OFFSET);
}

// Page fault handler
extern "C" void
pg_fault(cpu_ctx* registers, void* userptr)
{
    uint64_t cr2;
    asm("mov %%cr2, %0"
        : "=r"(cr2));

    log("    Linear Address: %x\n    Conditions:\n", cr2);

    // See Intel x86 SDM Volume 3 Chapter 4.7
    uint64_t error_code = registers->error_code;
    if (((error_code) & (1 << (0))))
        log("    - Page level protection violation\n");
    else
        log("    - Non-present page\n");

    if (((error_code) & (1 << (1))))
        log("    - Write\n");
    else
        log("    - Read\n");

    if (((error_code) & (1 << (2))))
        log("    - User access\n");
    else
        log("    - Supervisor access\n");

    if (((error_code) & (1 << (3))))
        log("    - Reserved bit set\n");

    if (((error_code) & (1 << (4))))
        log("    - Instruction fetch\n");

    if (((error_code) & (1 << (5))))
        log("    - Protection key violation\n");

    if (((error_code) & (1 << (15))))
        log("    - SGX violation\n");

    asm("cli; hlt");
}

// ====================================
//             Paging Code
// ====================================

namespace mm::vmm {
void init()
{
    mm::vmm::check_and_init(true);
    kspace.init();

    // Map the Kernel
    uint64_t virt_start = HIGHERHALF_OFFSET;
    uint64_t phys_start = 0x0;
    for (size_t i = 0; i < (1024 * 20); i++) {
        kspace.map(phys_start, virt_start,
            PAGE_PRESENT | PAGE_WRITEABLE | PAGE_GLOBAL);

        virt_start += 0x1000;
        phys_start += 0x1000;
    }

    // Map everything else
    struct stivale2_struct_tag_memmap* map_tag;
    map_tag = (stivale2_struct_tag_memmap*)arch::stivale2_get_tag(
        STIVALE2_STRUCT_TAG_MEMMAP_ID);

    for (size_t i = 0; i < map_tag->entries; i++) {
        struct stivale2_mmap_entry entry = map_tag->memmap[i];
        for (uint64_t g = 0; g < NUM_PAGES(entry.length) * PAGE_SIZE;
             g += PAGE_SIZE)
            kspace.map(entry.base + g, entry.base + MEM_VIRT_OFFSET + g,
                PAGE_PRESENT | PAGE_WRITEABLE);
    }

    flush_buffer();

    kspace.load();
}

void aspace::init()
{
    info = (PML4*)((uint64_t)mm::pmm::get(1) + MEM_VIRT_OFFSET);
    _memset((uint64_t)info, 0, 4096);
    pid = mm::pcid::alloc();
}

void aspace::load()
{
    uint64_t val = (uint64_t)info - MEM_VIRT_OFFSET;

    uint64_t cr3 = val;
    mm::pcid::apply(cr3, pid);
    if (mmu_features[0]) {
        cr3 |= (1ull << 63); // Don't invalidate this PCID (kinda defeats the purpose of PCIDs if enabled)
    }

    writecr("cr3", cr3);
}

void aspace::clean()
{
    if (info == nullptr)
        return;

    for (uint64_t loop_index = 0; loop_index < PAGE_DIR_COUNT; loop_index++) {
        uint64_t pdpt_entry = info->entries[loop_index];

        if (pdpt_entry & (1ull << 0)) {
            // Don't free huge pages
            if (!(pdpt_entry & (1ull << 7))) {
                uint64_t pdpt_addr = ((pdpt_entry & 0x000FFFFFFFFFF000) + MEM_VIRT_OFFSET);
                clean_pdpt((mm::vmm::PDPT*)pdpt_addr);

                mm::pmm::free((void*)(pdpt_entry & 0x000FFFFFFFFFF000), 1);
            }
        }
    }

    mm::pmm::free((void*)((uint64_t)info - MEM_VIRT_OFFSET), 1);
}

void aspace::clone(mm::vmm::aspace* new_space)
{
    // Start with a fresh address space
    new_space->clean();
    new_space->init();

    PML4* new_pml4 = (PML4*)new_space->get_root();

    for (uint64_t i = 0; i < PAGE_DIR_COUNT / 2; i++) {
        uint64_t old_entry = info->entries[i];

        uint64_t entry_flags = get_flags(old_entry);
        if (entry_flags & (1ull << 0)) {
            if (!(entry_flags & (1ull << 7))) {
                uint64_t pdpt = (uint64_t)clone_pdpt((PDPT*)((uint64_t)get_frame(old_entry) + MEM_VIRT_OFFSET));

                uint64_t new_entry = 0;
                frame_set(new_entry, pdpt);
                set_flags(new_entry, get_flags(old_entry));

                new_pml4->entries[i] = new_entry;
            } else {
                log("mm/vmm: Huge Pages are not Supported and are considered reserved!\n");
                new_pml4->entries[i] = old_entry; // Copy anyways
            }
        } // TODO: Implement Page Swapping
    }

    // Plain Copy the higher half of the address space

    for (uint64_t i = PAGE_DIR_COUNT / 2; i < PAGE_DIR_COUNT; i++) {
        uint64_t old_entry = info->entries[i];

        new_pml4->entries[i] = old_entry;
    }
}

bool mm::vmm::aspace::map(uint64_t phys, uint64_t virt, uint64_t flags,
    cache_type cache, int pku)
{
    uint64_t pml4_index = index_pml4(virt);
    uint64_t pdpt_index = index_pdpt(virt);
    uint64_t pd_index = index_pd(virt);
    uint64_t pt_index = index_pt(virt);

    uint64_t raw_flags = 0;
    if (flags & PAGE_PRESENT)
        raw_flags |= (1 << raw_page_present);
    if (flags & PAGE_GLOBAL)
        raw_flags |= (1 << raw_page_global);
    if (flags & PAGE_NX)
        raw_flags |= ((uint64_t)1 << raw_page_no_execute);
    if (flags & PAGE_WRITEABLE)
        raw_flags |= (1 << raw_page_writeable);
    if (flags & PAGE_USER)
        raw_flags |= (1 << raw_page_user);

    PML4* pml4_raw = this->info;
    uint64_t pml4_entry = pml4_raw->entries[pml4_index];
    if (!((pml4_entry) & (1 << (raw_page_present)))) {
        uint64_t new_pml4 = 0;
        uint64_t pdpt = (uint64_t)mm::pmm::get(1);
        _memset(pdpt + MEM_VIRT_OFFSET, 0, 4096);
        frame_set(new_pml4, pdpt);

        new_pml4 |= 0b111; // Present | ReadWrite | User
        pml4_raw->entries[pml4_index] = new_pml4;
        pml4_entry = new_pml4;
    }

    PDPT* pdpt = (PDPT*)((uint64_t)get_frame(pml4_entry) + MEM_VIRT_OFFSET);
    uint64_t pdpt_entry = pdpt->entries[pdpt_index];
    if (!((pdpt_entry) & (1 << (raw_page_present)))) {
        uint64_t new_pdpt = 0;
        uint64_t pd = (uint64_t)mm::pmm::get(1);
        _memset(pd + MEM_VIRT_OFFSET, 0, 4096);
        frame_set(new_pdpt, pd);

        new_pdpt |= 0b111; // Present | ReadWrite | User
        pdpt->entries[pdpt_index] = new_pdpt;
        pdpt_entry = new_pdpt;
    }

    PD* pd = (PD*)((uint64_t)get_frame(pdpt_entry) + MEM_VIRT_OFFSET);
    uint64_t pd_entry = pd->entries[pd_index];
    if (!((pd_entry) & (1 << (raw_page_present)))) {
        uint64_t new_pd = 0;
        uint64_t pt = (uint64_t)mm::pmm::get(1);
        _memset(pt + MEM_VIRT_OFFSET, 0, 4096);
        frame_set(new_pd, pt);

        new_pd |= 0b111; // Present | ReadWrite | User
        pd->entries[pd_index] = new_pd;
        pd_entry = new_pd;
    }

    PT* pt = (PT*)((uint64_t)get_frame(pd_entry) + MEM_VIRT_OFFSET);

    uint64_t pt_entry = 0;
    frame_set(pt_entry, phys);
    set_flags(pt_entry, raw_flags);
    pt_entry |= cache2pat(cache);

    if (pku > 0)
        WRITETO(pt_entry, 59ull, 4ull, (uint64_t)pku);

    pt->entries[pt_index] = pt_entry;

    return true;
}

uint64_t
aspace::virt2phys(uint64_t virt)
{
    uint64_t pml4_index = index_pml4(virt);
    uint64_t pdpt_index = index_pdpt(virt);
    uint64_t pd_index = index_pd(virt);
    uint64_t pt_index = index_pt(virt);

    uint64_t pml4_entry = info->entries[pml4_index];
    if (((pml4_entry) & (1 << (raw_page_present)))) {
        PDPT* pdpt
            = (PDPT*)get_frame(info->entries[pml4_index]) + MEM_VIRT_OFFSET;
        uint64_t pdpt_entry = pdpt->entries[pdpt_index];
        if (((pdpt_entry) & (1 << (raw_page_present)))) {
            PD* pd
                = (PD*)get_frame(pdpt->entries[pdpt_index]) + MEM_VIRT_OFFSET;
            uint64_t pd_entry = pd->entries[pd_index];
            if (((pd_entry) & (1 << (raw_page_present)))) {
                PT* pt
                    = (PT*)get_frame(pd->entries[pd_index]) + MEM_VIRT_OFFSET;
                uint64_t pt_entry = pt->entries[pt_index];
                if (((pt_entry) & (1 << (raw_page_present)))) {
                    return get_frame(pt_entry);
                }
            }
        }
    }

    return 0;
}

void aspace::unmap(uint64_t virt)
{
    uint64_t pml4_index = index_pml4(virt);
    uint64_t pdpt_index = index_pdpt(virt);
    uint64_t pd_index = index_pd(virt);
    uint64_t pt_index = index_pt(virt);

    PML4* pml4_raw = this->info;
    uint64_t pml4_entry = pml4_raw->entries[pml4_index];
    if (!((pml4_entry) & (1 << (raw_page_present)))) {
        return;
    }

    PDPT* pdpt = (PDPT*)((uint64_t)get_frame(pml4_entry) + MEM_VIRT_OFFSET);
    uint64_t pdpt_entry = pdpt->entries[pdpt_index];
    if (!((pdpt_entry) & (1 << (raw_page_present)))) {
        return;
    }

    PD* pd = (PD*)((uint64_t)get_frame(pdpt_entry) + MEM_VIRT_OFFSET);
    uint64_t pd_entry = pd->entries[pd_index];
    if (!((pd_entry) & (1 << (raw_page_present)))) {
        return;
    }

    PT* pt = (PT*)((uint64_t)get_frame(pd_entry) + MEM_VIRT_OFFSET);

    uint64_t pt_entry = 0;
    pt->entries[pt_index] = pt_entry;

    asm volatile("invlpg (%0)" ::"r"(virt)
                 : "memory");
}
} // namespace mm::vmm

// ====================================
//           PCID Implentation
// ====================================

// Use the lru algorithm for the PCID Cache
static uint64_t pcid_cache[16] = { 1 };

namespace mm::pcid {
uint64_t alloc()
{
    uint64_t cur_pcid;
    uint64_t lru_pcid;
    uint64_t use_pcid = ~0ull;

    vmm_mutex.lock();

    for (cur_pcid = 0; cur_pcid < 16; cur_pcid++) {

        if (pcid_cache[cur_pcid] < use_pcid) {

            lru_pcid = cur_pcid;
            use_pcid = pcid_cache[cur_pcid];
        }
    }

    pcid_cache[lru_pcid]++;

    vmm_mutex.unlock();
    return lru_pcid;
}

void free(uint64_t id)
{
    vmm_mutex.lock();
    pcid_cache[id]--;
    vmm_mutex.unlock();
}

void apply(uint64_t cr3, uint64_t pcid)
{
    WRITETO(cr3, 0, 11, pcid);
}
}

// ====================================
//             PKU/PKS Code
// ====================================

static uint32_t
_rdpkru_u32(bool mode)
{
    if (mode)
        return rdmsr(IA32_PKS);

    uint32_t ecx = 0;
    uint32_t edx, pkru;

    asm volatile("rdpkru\n\t"
                 : "=a"(pkru), "=d"(edx)
                 : "c"(ecx));
    return pkru;
}

static void
_wrpkru(bool mode, uint32_t pkru)
{
    if (mode) {
        wrmsr(IA32_PKS, pkru);
        return;
    }

    uint32_t ecx = 0, edx = 0;

    asm volatile("wrpkru\n\t"
                 :
                 : "a"(pkru), "c"(ecx), "d"(edx));
}

static bool pku_map[16] = { true };
static bool pks_map[16] = { true };

namespace mm::pkey {
uint8_t
alloc(bool supervisor)
{
    if (supervisor) {
        for (int f = 0; f < 16; f++) {
            if (pks_map[f]) {
                pks_map[f] = false;
                return f;
            }
        }
    } else {
        for (int f = 0; f < 16; f++) {
            if (pks_map[f]) {
                pks_map[f] = false;
                return f;
            }
        }
    }

    log("WARNING: Out of %s Protection Keys\n",
        supervisor ? "Supervisor" : "User-Mode");
    return 0xFF; // Random Value that's greater than 16
}

void set_perm(uint8_t key, bool supervisor, bool access_disable,
    bool write_disable)
{
    uint32_t old_pkru, new_pkru = 0;

    if (supervisor) {
        old_pkru = _rdpkru_u32(true);
        if (access_disable) {
            new_pkru |= (0x01 << ((key)*2));
        } else {
            new_pkru &= ~(0x01 << ((key)*2));
        }

        if (write_disable) {
            new_pkru |= (0x02 << ((key)*2));
        } else {
            new_pkru &= ~(0x02 << ((key)*2));
        }

        _wrpkru(true, old_pkru | new_pkru);
    } else {
        old_pkru = _rdpkru_u32(false);

        if (access_disable) {
            new_pkru |= (0x01 << ((key)*2));
        } else {
            new_pkru &= ~(0x01 << ((key)*2));
        }

        if (write_disable) {
            new_pkru |= (0x02 << ((key)*2));
        } else {
            new_pkru &= ~(0x02 << ((key)*2));
        }

        _wrpkru(false, old_pkru | new_pkru);
    }
}

void free(uint8_t key, bool supervisor)
{
    if (supervisor) {
        pks_map[key] = true;
    } else {
        pku_map[key] = true;
    }
}
} // namespace mm::vmm

// ====================================
//            Misc VMM Code
// ====================================

namespace mm::vmm {
void check_and_init(bool bsp)
{
    vmm_mutex.lock();

    uint32_t eax, ebx, ecx, edx;
    arch::cpuid(1, eax, ebx, ecx, edx);

    if (bsp)
        log("x64/mmu: Features ( ");

    if (ecx & CPUID_PCID) {
        if (bsp) {
            log("PCID ");
            mmu_features[0] = true;
        }

        uint64_t cr4;
        readcr("cr4", &cr4);
        cr4 |= (1 << 17);
        writecr("cr4", cr4);
    }

    if(!arch::cpuid(7, 0, eax, ebx, ecx, edx))
	goto check_for_nx;

    if (ebx & CPUID_SMEP) {
        if (bsp) {
            log("SMEP ");
            mmu_features[1] = true;
        }

        // Enable right away
        uint64_t val;
        readcr("cr4", &val);
        val |= (1 << 20);
        writecr("cr4", val);
    }
    if (ebx & CPUID_SMAP) {
        if (bsp) {
            log("SMAP ");
            mmu_features[2] = true;
        }

        // Enable right away
        uint64_t val;
        readcr("cr4", &val);
        val |= (1 << 21);
        writecr("cr4", val);
    }
    if (ecx & CPUID_PKU) {
        if (bsp) {
            log("PKU ");
            mmu_features[3] = true;
        }

        uint64_t val;
        readcr("cr4", &val);
        val |= (1 << 22);
        writecr("cr4", val);

        // Free every key
        _wrpkru(false, 0);
    }

    if (ecx & (uint32_t)CPUID_PKS) {
        if (bsp) {
            log("PKS ");
            mmu_features[4] = true;
        }

        uint64_t cr4;
        readcr("cr4", &cr4);
        cr4 |= (1 << 24);
        writecr("cr4", cr4);

        // Free every key
        _wrpkru(true, 0);
    }

check_for_nx:
    if(!arch::cpuid(0x80000001, eax, ebx, ecx, edx))
	PANIC("x64/mmu: Unable to check for NX (Execute Disable), Required for boot!\n");

    if (edx & CPUID_NX) {
        if (bsp) {
            log("NX ");
            mmu_features[5] = true;
        }

        // Enable right away
        uint64_t val = rdmsr(0xC0000080);
        val |= (1 << 11);
        wrmsr(0xC0000080, val);
    }

    if (bsp)
        log(")\n");

    // set guard as early as possible
    mm::vmm::set_guard();

    // Enable Write Protection and Global Pages
    uint64_t cr0;
    readcr("cr0", &cr0);
    cr0 |= (1 << 16); // Write Protect
    writecr("cr0", cr0);

    readcr("cr4", &cr0);
    cr0 |= (1 << 7); // Page Global Enable
    writecr("cr4", cr0);

    // Add Page Fault Handler
    register_handler({ .vector = 14, .callback = &pg_fault });

    // Overwrite PAT with our value
    wrmsr(0x277, 0x06 | (0x01 << 8) | (0x04 << 16) | (0 << 24));

    vmm_mutex.unlock();
}

aspace*
kernel_space() { return &kspace; }
} // namespace mm::vmm
