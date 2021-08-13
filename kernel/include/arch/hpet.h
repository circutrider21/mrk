#pragma once

#include <mrk/acpi.h>

namespace arch::hpet {
    typedef struct [[gnu::packed]] {
        acpi::sdt_hdr hdr;

        uint8_t hardware_rev_id;
        uint8_t comparator_count : 5;
        uint8_t counter_size : 1;
        uint8_t reserved : 1;
        uint8_t legacy_replace : 1;
        uint16_t pci_vendor_id;

        // base address
        acpi::generic_addr_struct base_addr;

        uint8_t hpet_number;
        uint16_t minimum_tick;
        uint8_t page_protection;
    } table;

    constexpr uint64_t cap_id = 0;
    constexpr uint64_t gen_conf = 0x10;
    constexpr uint64_t int_status = 0x20;
    constexpr uint64_t main_counter_data = 0xF0;
    
    void init();
    uint64_t get_nanos();
    void sleep(uint64_t us);
}

