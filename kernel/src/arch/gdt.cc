#include <arch/cpu.h>

gdt kgdt;

void gdt::init()
{
    entry_null = GDT_ENTRY_NULL;
    entry_kcode = GDT_ENTRY_KERNEL_CODE;
    entry_kdata = GDT_ENTRY_KERNEL_DATA;
    entry_ucode = GDT_ENTRY_USER_CODE;
    entry_udata = GDT_ENTRY_USER_DATA;

    gt.base = (uint64_t)this;
    gt.limit = sizeof(gdt) - 1;

    gt.load();
}

extern "C" void __lgdt(void* ptr);

void gdtr::load() { __lgdt((void*)this); }
