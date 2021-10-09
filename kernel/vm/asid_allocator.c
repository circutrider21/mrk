#include <lib/bitops.h>
#include <arch/vmm.h>

#ifdef __aarch64__
  #define ASID_MAX_SIZE 65536
#elif  __x86_64__
  #define ASID_MAX_SIZE 4096
#else
  #error "Unknown Maximum Size of ASID (Address Space IDentifier)"
#endif

static bitmap_t asid_table;
static uint64_t _bitmap[ASID_MAX_SIZE / 8];

uint16_t vm_asid_alloc() {
  for(int i = 10; i < ASID_MAX_SIZE; i++) {
    if (bitmap_get(&asid_table, i) == 0) {
      bitmap_set(&asid_table, i);
      return i;
    }
  }

  // ASID 2 is used when no other ASIDs are available (Note: It can be shared with other threads)
  return 2;
}

void vm_asid_free(uint16_t asid) {
  bitmap_clear(&asid_table, asid);

  // Invalidate the ASID
  __invasid(asid);
}

void setup_asid() {
  asid_table.map = (uint8_t*)_bitmap;
  asid_table.size = ASID_MAX_SIZE / 8;
  bitmap_fill(&asid_table, 0);

  // Lock the first 10 ASIDs (for kernel/core driver use)
  for(int i = 0; i < 10; i++) {
    bitmap_set(&asid_table, i);
  }
}
