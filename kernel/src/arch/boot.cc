#include <cstddef>
#include <internal/stivale2.h>
#include <internal/version.h>
#include <klib/builtin.h>

#include <arch/apic.h>
#include <arch/arch.h>
#include <arch/cpu.h>
#include <arch/idt.h>
#include <arch/smp.h>
#include <arch/hpet.h>

#include <mrk/acpi.h>
#include <mrk/alloc.h>
#include <mrk/log.h>
#include <mrk/pmm.h>
#include <mrk/proc.h>
#include <mrk/vmm.h>

static uint8_t _stack[8192];
void (*term_write)(const char* string, size_t length);
static struct stivale2_struct* bootlog;

// The stivale2 tags...
static struct stivale2_header_tag_smp smp_hdr_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_SMP_ID,
        .next = 0 },

    // Enable x2apic (if supported)
    .flags = (1 << 0),
};

static struct stivale2_header_tag_terminal terminal_hdr_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_TERMINAL_ID,
        .next = (uint64_t)&smp_hdr_tag },

    .flags = 0
};

static struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        .next = (uint64_t)&terminal_hdr_tag },

    .framebuffer_width = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp = 0
};

namespace arch {
void* stivale2_get_tag(uint64_t id)
{
    struct stivale2_tag* current_tag = (struct stivale2_tag*)bootlog->tags;
    for (;;) {
        if (current_tag == nullptr) {
            return nullptr;
        }

        if (current_tag->identifier == id) {
            return current_tag;
        }

        // Get a pointer to the next tag in the linked list and repeat.
        current_tag = (struct stivale2_tag*)current_tag->next;
    }
}
}

static void init_thread(uint64_t magic)
{
    log("init: kernel startup is now complete, halting...\n");
    
    if (magic != 0xDEADBEEF)
        PANIC("Invalid thread magic");

    for (;;)
        ;
}

static void boot_banner()
{
    log("Mrk Kernel (v%s) (Date: %s) ", MRK_VERSION, __DATE__);
#if defined(__clang__)
    log("(Clang v%d.%d.%d)\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#else
#if defined(__GNUC__)
    log("(GCC v%d.%d.%d)\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif
#endif
    log("Copyright (c) 2021 Yusuf M, All Rights Reserved.\n\n");
}

void _start(struct stivale2_struct* stivale2_struct)
{
    bootlog = stivale2_struct;

    struct stivale2_struct_tag_terminal* term_str_tag;
    term_str_tag = (struct stivale2_struct_tag_terminal*)arch::stivale2_get_tag(STIVALE2_STRUCT_TAG_TERMINAL_ID);

    if (term_str_tag == nullptr) {
        PANIC("stivale2 terminal tag not found!\n");
    }

    void* term_write_ptr = (void*)term_str_tag->term_write;

    term_write = (void (*)(const char*, size_t))term_write_ptr;

    boot_banner();

    mm::pmm::init();

    arch::simd::init();

    mm::pmm::dump();
    mm::vmm::init();

    kgdt.init();
    arch::idt::init();

    acpi::init();
    arch::hpet::init();

    smp::init_others();
    arch::apic::init();
    arch::cpu::init();

    proc::init();
    proc::create_kthread((uint64_t)&init_thread, 0xDEADBEEF, true);

    // ADD CODE TO init_thread, NOT HERE!
    asm volatile("sti");
    for (;;)
        ;
}

__attribute__((section(".stivale2hdr"), used)) static struct stivale2_header stivale_hdr = {
    .entry_point = (uint64_t)&_start,
    .stack = (uintptr_t)_stack + sizeof(_stack),
    // Bit 1, if set, causes the bootloader to return to us pointers in the
    // higher half, which we likely want.
    .flags = (1 << 1),
    .tags = (uintptr_t)&framebuffer_hdr_tag
};
