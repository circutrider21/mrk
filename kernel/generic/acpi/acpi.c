#include <generic/acpi.h>
#include <generic/log.h>
#include <lib/stivale2.h>
#include <lib/builtins.h>
#include <stddef.h>
#include <stdbool.h>

static bool xsdt_found;
static acpi_rsdt *rsdt;

void init_acpi() {
    struct stivale2_struct_tag_rsdp* rk = (struct stivale2_struct_tag_rsdp*)stivale2_query(STIVALE2_STRUCT_TAG_RSDP_ID);
    acpi_rsdp* rsdp = (acpi_rsdp*)rk->rsdp;

    log("acpi: Revision: %u\n", rsdp->rev);
    if (rsdp->rev >= 2 && rsdp->xsdt_addr) {
        xsdt_found = true;
        rsdt = (acpi_rsdt*)((uintptr_t)rsdp->xsdt_addr);
	log("acpi: Found XSDT at 0x%X\n", (uintptr_t)rsdt);
    } else {
	xsdt_found = false;
	rsdt = (acpi_rsdt*)((uintptr_t)rsdp->rsdt_addr);
	log("acpi: Found RSDT at 0x%X\n", (uintptr_t)rsdt);
    }

    // Dump all tables
    char buf[5] = { 0 };
    buf[4] = 0;

    for (size_t i = 0; i < rsdt->sdt.length - sizeof(acpi_sdt); i++) {
        uintptr_t entry;
        if (xsdt_found)
            entry = (uintptr_t)(((uint64_t*)rsdt->ptrs_start)[i]);
	else
	    entry = (uintptr_t)(((uint32_t*)rsdt->ptrs_start)[i]);

	if (!entry)
	    continue;

        acpi_sdt* h = (acpi_sdt*)entry;
	for (int g = 0; g < 4; g++)
	    buf[g] = h->signature[g];

	log("acpi: \"%s\" [0x%X %u]\n", buf, entry, h->length);
    }
}

void *acpi_query(const char *signature, int index) {
    int cnt = 0;
    for (size_t i = 0; i < rsdt->sdt.length - sizeof(acpi_sdt); i++) {
        acpi_sdt *ptr;
        if (xsdt_found)
	    ptr = (acpi_sdt*)(((uint64_t*)rsdt->ptrs_start)[i]);
        else
	    ptr = (acpi_sdt*)(((uint32_t*)rsdt->ptrs_start)[i]);
			    
        if (memcmp(ptr->signature, signature, 4) == 0 && cnt++ == index) {
	    log("acpi: Found \"%s\" at %X\n", signature, ptr);
            return (void*)ptr;
        }
    }

    log("acpi: \"%s\" not found\n", signature);
    return NULL;
}

