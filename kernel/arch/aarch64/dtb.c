#include <arch/dtb.h>
#include <lib/stivale2.h>
#include <generic/log.h>

static uint32_t* dtb_data;
static uintptr_t dtb_size;
static struct devicetree_header dtb_hdr;

void init_dtb() {
    struct stivale2_struct_tag_dtb* tag_dtb = stivale2_query(STIVALE2_STRUCT_TAG_DTB);
    dtb_data = (uint32_t*)tag_dtb->addr;
    dtb_size = tag_dtb->size;

    // Fill in the header
    dtb_hdr.magic              = bswap_32(*(dtb_data));
    dtb_hdr.total_size         = bswap_32(*(dtb_data + 1));
    dtb_hdr.data_offset        = bswap_32(*(dtb_data + 2));
    dtb_hdr.string_offset      = bswap_32(*(dtb_data + 3));
    dtb_hdr.memmap_offset      = bswap_32(*(dtb_data + 4));
    dtb_hdr.version            = bswap_32(*(dtb_data + 5));
    dtb_hdr.last_comp_version  = bswap_32(*(dtb_data + 6));
    dtb_hdr.boot_cpu_num       = bswap_32(*(dtb_data + 7));
    dtb_hdr.string_size        = bswap_32(*(dtb_data + 8));
    dtb_hdr.data_size          = bswap_32(*(dtb_data + 9));

    if(dtb_hdr.version != 17 || dtb_hdr.magic != 0xD00DFEED || dtb_hdr.total_size > dtb_size)
	log("dtb: Bogus header values\n");

    // Iterate through reserved memory ranges
    uint32_t* memmap = ((uintptr_t)dtb_data + dtb_hdr.memmap_offset);
    for(int i  = 0;;i++) {
	uint64_t memory = bswap_64(*((uint64_t*)memmap++));
	uint64_t memory_len = bswap_64(*((uint64_t*)memmap++));

	if(memory == 0 && memory_len == 0)
	    break;

	log("dtb: Reserved memory range [0x%p 0x%x]\n", memory, memory_len);
    }

    // Print model name
    fdt_node my_nod = find_attrib(NULL, "model");
    if (my_nod.data == NULL)
        log("dtb: Model - Unknown\n");
    else
	    log("dtb: Model - %s\n", my_nod.data);
}

fdt_node find_attrib(char* node_name, char* attrib_name) {
    uint32_t* current = ((uintptr_t)dtb_data + dtb_hdr.data_offset);
    int cur_depth = 0;
    int found_depth = 0;

    for(;;) {
        uint32_t node_type = bswap_32(*(current++));

	    switch(node_type) {
            case 0x1 : { // FDT_BEGIN_NODE
                char* str = (char*)current;
		        int len = strlen(str);
		        // log("dtb: FDT_BEGIN_NODE [%s]\n", *str == '\0' ? "root" : str);

		        cur_depth++;
                if (found_depth == 0 && len >= strlen(node_name)) {
		        if (memcmp(str, node_name, strlen(node_name)) == 0)
                    found_depth = cur_depth;
                }

		        // Check if root node is wanted
		        if (found_depth == 0 && node_name == NULL && *str == '\0') {
		            found_depth = cur_depth;
		        }

                current += (len + 4) / 4;
		        break;
	        }
	        case 0x2: { // FDT_END_NODE
                if(found_depth) {
                    if(found_depth == cur_depth)
		                found_depth = 0;
		        }

		        cur_depth--;
		        break;
	        }
	        case 0x3 : { // FDT_PROP
	            uint32_t prop_len = bswap_32(*(current));
		        uint32_t prop_name_offset = bswap_32(*(current + 1));

		        char* name = ((uintptr_t)dtb_data + dtb_hdr.string_offset + prop_name_offset);
		        // log("dtb: FDT_PROP [name: %s, size: %x]\n", name, prop_len);

		        if(found_depth) {
                    if(found_depth == cur_depth) {
                        if(memcmp(name, attrib_name, strlen(name)) == 0)
			                return (fdt_node){ .data = ((uint8_t*)current + 2), .len = prop_len };
		            }
		        }
		        prop_len += 3;
                current += prop_len / 4 + 2;
		        break;
	        }
	        case 0x4: break; // FDT_NOP
	        case 0x9: goto out; // FDT_END
	    }
    }

out:
    return (fdt_node){ .data = NULL, .len = 0 };
}

