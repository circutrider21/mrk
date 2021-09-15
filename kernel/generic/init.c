#include <lib/stivale2.h>
#include <vm/vm.h>
#include <generic/fbcon.h>
#include <generic/log.h>
#include <generic/acpi.h>
#include <arch/arch.h>
#include <arch/debug.h>

// The stack is actually created inside the linker script, so import the symbols here.
extern char __stack_high[];
static struct stivale2_struct* bootinfo;

#ifdef __aarch64__
extern struct stivale2_struct_tag_mmio32_uart* mmio_struct;
#endif

// The stivale2 tags...
static struct stivale2_header_tag_smp smp_hdr_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_SMP_ID,
        .next = 0
    },
    .flags = 0,
};

static struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        .next = (uint64_t)&smp_hdr_tag
    },
    // Use the native resolution, since it looks better
    .framebuffer_width  = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp    = 0
};

__attribute__((section(".stivale2hdr"), used))
static struct stivale2_header stivale_hdr = {
    .entry_point = 0,
    .stack = (uint64_t)__stack_high,
    .flags = (1 << 1),
    .tags = (uintptr_t)&framebuffer_hdr_tag
};

void* stivale2_query(uint64_t id) {
    struct stivale2_tag* current_tag = (struct stivale2_tag*)bootinfo->tags;
    for (;;) {
        if (current_tag == NULL) {
            return NULL;
        }

        if (current_tag->identifier == id) {
            return current_tag;
        }

        // Get a pointer to the next tag in the linked list and repeat.
        current_tag = (struct stivale2_tag*)current_tag->next;
    }
}

void mrk_entry(struct stivale2_struct* stivale2_struct) {
    bootinfo = stivale2_struct;

#ifdef __aarch64__
    mmio_struct = (struct stivale2_struct_tag_mmio32_uart*)stivale2_query(0xb813f9b8dbc78797);
    // Put a \n as a marker between sabaton's log and mrk's log
    debug_putc('\n');
#endif

    init_fbcon(); // The fbcon is manually init'ed even though its in the initgraph
    log("Hello from mrk!\n");
    arch_init_early();
    init_acpi();
    vm_init();
    arch_init();
    log("init: boot sequence complete, halting!\n");

#ifdef __aarch64__
    asm volatile ("wfi");
    for(;;);
#endif
#ifdef __x86_64__
    asm volatile ("cli");
    asm volatile ("hlt");

    // Just for safety
    for(;;);
#endif
}

