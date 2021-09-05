#ifndef ARCH_DTB_H
#define ARCH_DTB_H

#include <stdint.h>

// DeviceTree Blob Header

struct devicetree_header {
    uint32_t magic;
    uint32_t total_size;
    uint32_t data_offset;
    uint32_t string_offset;
    uint32_t memmap_offset;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpu_num;
    uint32_t string_size;
    uint32_t data_size;
};

typedef struct {
    uint8_t* data;
    uint32_t len;
} fdt_node;

#define bswap_32(k) __builtin_bswap32(k)
#define bswap_64(k) __builtin_bswap64(k)

void init_dtb();
fdt_node find_attrib(char* node_name, char* attrib_name);

#endif // ARCH_DTB_H

