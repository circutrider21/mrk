#include <lib/stivale2.h>
#include <generic/fbcon.h>
#include <generic/initgraph.h>
#include <generic/log.h>

// The stack is actually created inside the linker script, so import the symbols here.
extern char __stack_high[];
extern struct stivale2_struct_tag_mmio32_uart* mmio_struct;
struct stivale2_struct* bootinfo;


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

void mrk_start_aa64(struct stivale2_struct* stivale2_struct) {
    bootinfo = stivale2_struct;
    mmio_struct = (struct stivale2_struct_tag_mmio32_uart*)stivale2_query(0xb813f9b8dbc78797);

    // Put a \n as a marker between sabaton's log and mrk's log
    *((volatile uint32_t*)mmio_struct->addr) = '\n';

    init_fbcon();
    log("Hello from mrk!\n");
    initgraph_dump();

    asm volatile ("wfi");
    for(;;);
}

