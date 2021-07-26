#pragma once

#include <cstdint>

namespace acpi {
    struct __attribute__((packed)) rsdp {
        char signature[8];
        uint8_t checksum;
        char oem_id[6];
        uint8_t revision;
        uint32_t rsdt_address;
    };

    struct __attribute__((packed)) xsdp {
        char signature[8];
        uint8_t checksum;
        char oem_id[6];
        uint8_t revision;
        uint32_t rsdt_address;
        uint32_t length;
        uint64_t xsdt_address;
        uint8_t extended_checksum;
        uint8_t reserved[3];
    };

    struct __attribute__((packed)) sdt_hdr {
        char signature[4];
        uint32_t length;
        uint8_t revision;
        uint8_t checksum;
        char oem_id[6];
        char oem_tableid[8];
        uint32_t oem_revision;
        uint32_t creator_id;
        uint32_t creator_revision;
    };

    struct __attribute__((packed)) rsdt {
        acpi::sdt_hdr header;
        uint32_t tables[];
    };

    struct __attribute__((packed)) xsdt {
        acpi::sdt_hdr header;
        uint64_t tables[];
    };    

    struct __attribute__((packed)) generic_addr_struct {
        uint8_t id;
        uint8_t bit_width;
        uint8_t bit_offset;
        uint8_t access_size;
        uint64_t address;
    };

    struct table {
        acpi::sdt_hdr header;
        uint8_t data[];
    };

    void init();
    void* get_table(const char* signature, uint64_t index);
}

