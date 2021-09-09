#include <arch/arch.h>
#include <arch/dtb.h>

extern void __load_vectors();
void arch_init_early() {
    __load_vectors();
    init_dtb();

    // Run some safety checks on the mmu...
    uint64_t mmu_id0;
    asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r" (mmu_id0));

    if (mmu_id0 & (0xf << 28))
        log("arch: 4K Memory Transation Granule NOT SUPPORTED!\n");

    if ((mmu_id0 & 0xF) < 1)
	log("arch: 48-bit Address width NOT SUPPORTED! (default: %u)\n", (mmu_id0 & 0xF));

    // Setup the MAIR register
    uint64_t new_mair = 0b11111111 |          // Normal, Write-back RW-Allocate non-transient
	    		(0b00001100 << 8) |   // Device, GRE
			(0b00000000 << 16)|   // Device, nGnRnE
			(0b00000100 << 24)|   // Device, nGnRE
			(0b01000100UL << 32); // Normal Non-cacheable
    
    asm volatile ("msr mair_el1, %0" :: "r" (new_mair));
}

