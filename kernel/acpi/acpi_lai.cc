#include <internal/builtin.h>
#include <lai/host.h>
#include <mrk/alloc.h>
#include <mrk/arch.h>
#include <mrk/log.h>
#include <mrk/pmm.h>
#include <mrk/vmm.h>

extern "C" {

void* laihost_malloc(size_t sz)
{
    void* ptr = mm::alloc(sz);
    log("TRACE: [Alloc] %d -> %x\n", sz, ptr);
    return ptr;
}

void* laihost_realloc(void* ptr, size_t new_size, size_t old_size)
{
    // TODO: Use my own realloc instead of this temp fix
    if (!ptr && !new_size) {
        return NULL;
    }
    if (!new_size) {
        mm::free(ptr);
        return NULL;
    }
    if (!ptr) {
        return mm::alloc(new_size);
    }
    void* ret = mm::alloc(new_size);
    if (!ret) {
        return NULL;
    }
    _memcpy(ret, ptr, old_size);
    log("TRACE: [Realloc] %d -> %x\n", new_size, ret);
    return ret;
}

void laihost_free(void* ptr, size_t sz)
{
    (void)sz;
    mm::free(ptr);
}

void laihost_log(int level, const char* msg)
{
    switch (level) {
    case LAI_DEBUG_LOG:
        log("lai: [Debug] %s\n", msg);
        break;
    case LAI_WARN_LOG:
        log("lai: [Warning] %s\n", msg);
        break;
    default:
        log("lai: %s\n", msg);
        break;
    }
}

void laihost_panic(const char* msg)
{
    PANIC("PANIC: [lai] %s\n", (char*)msg);
}

void* laihost_scan(const char* signature, size_t index)
{
    void* nik = (void*)acpi::get_table(signature, index);
    return nik;
}

void* laihost_map(size_t addr, size_t bytes)
{
    uint64_t n_pages = ROUND_UP(bytes, PAGE_SIZE); // Ceil it to make sure we have all bytes allocated in case (bytes & 0xFFF) != 0

    for (size_t i = 0; i < n_pages; i++) {
        uint64_t offset = (i * PAGE_SIZE);
        mm::vmm::kernel_space()->map((addr + offset), (addr + offset + MEM_VIRT_OFFSET), PAGE_PRESENT | PAGE_WRITEABLE | PAGE_NX | PAGE_GLOBAL);
    }

    return (void*)((uint64_t)addr + MEM_VIRT_OFFSET);
}

void laihost_unmap(void* addr, size_t bytes)
{
    uint64_t n_pages = ROUND_UP(bytes, PAGE_SIZE);

    for (size_t i = 0; i < n_pages; i++) {
        uint64_t offset = (i * PAGE_SIZE);
        mm::vmm::kernel_space()->unmap((uint64_t)(addr + offset + MEM_VIRT_OFFSET));
    }
}

void laihost_outb(uint16_t port, uint8_t val) { __outb(port, val); }
void laihost_outw(uint16_t port, uint16_t val) { __outw(port, val); }
void laihost_outd(uint16_t port, uint32_t val) { __outd(port, val); }

uint8_t laihost_inb(uint16_t port) { return __inb(port); }
uint16_t laihost_inw(uint16_t port) { return __inw(port); }
uint32_t laihost_ind(uint16_t port) { return __ind(port); }

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint8_t value)
{
    log("lai: laihost_pci_* not implemented!\n");
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
    log("lai: laihost_pci_* not implemented!\n");
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value)
{
    log("lai: laihost_pci_* not implemented!\n");
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
    log("lai: laihost_pci_* not implemented!\n");
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value)
{
    log("lai: laihost_pci_* not implemented!\n");
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
    log("lai: laihost_pci_* not implemented!\n");
}

void laihost_sleep(uint64_t ms)
{
    arch::sleep(ms);
}

void memcpy(void* dest, const void* src, size_t n)
{
    _memcpy(dest, src, n);
}

void memset(void* s, int c, size_t n)
{
    _memset((uint64_t)s, c, n);
}

int memcmp(const void* s1, const void* s2, size_t n)
{
    return _memcmp(s1, s2, n);
}
}
