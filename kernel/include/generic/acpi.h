#ifndef GENERIC_ACPI_H
#define GENERIC_ACPI_H

#include <stdint.h>

typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t rev;
    uint32_t rsdt_addr;
    
    // rev >= 2
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t ext_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) acpi_rsdp;

typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t rev;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_rev;
    uint32_t creator_id;
    uint32_t creator_rev;
} __attribute__((packed)) acpi_sdt;

typedef struct {
    acpi_sdt sdt;
    char ptrs_start[];
} __attribute__((packed)) acpi_rsdt;

void init_acpi();

#endif // GENERIC_ACPI_H

