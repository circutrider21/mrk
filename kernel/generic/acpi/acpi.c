#include <generic/acpi.h>
#include <generic/log.h>
#include <lib/stivale2.h>
#include <lib/builtins.h>
#include <vm/virtual.h>
#include <vm/vm.h>

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
        rsdt = (acpi_rsdt *)((uintptr_t)rsdp->xsdt_addr + VM_MEM_OFFSET);
	    log("acpi: Found XSDT at 0x%X\n", (uintptr_t)rsdt);
    } else {
	    xsdt_found = false;
	    rsdt = (acpi_rsdt *)((uintptr_t)rsdp->rsdt_addr + VM_MEM_OFFSET);
	    log("acpi: Found RSDT at 0x%X\n", (uintptr_t)rsdt);
    }

    log("acpi: dumping tables...\n");
    log("    %s %s %s %s\n", "Signature", "Rev", "OEMID", "Address");
    for (size_t i = 0; i < rsdt->sdt.length - sizeof(acpi_sdt); i++) {
        uintptr_t entry;
        if (xsdt_found)
            entry = (uintptr_t)(((uint64_t*)rsdt->ptrs_start)[i]);
	    else
	        entry = (uintptr_t)(((uint32_t*)rsdt->ptrs_start)[i]);

    	if (!entry)
	       continue;

        acpi_sdt* h = (acpi_sdt*)entry;

        log("    %c%c%c%c      %d   %c%c%c%c%c%c0x%p\n",
            h->sig[0], h->sig[1], h->sig[2], h->sig[3],
            h->rev,
            h->oem_id[0], h->oem_id[1], h->oem_id[2], h->oem_id[3], h->oem_id[4], h->oem_id[5] ? h->oem_id[5] : ' ',
            (uint64_t)entry + VM_MEM_OFFSET
        );
    }
}

void *acpi_query(const char *signature, int index) {
    int cnt = 0;
    for (size_t i = 0; i < rsdt->sdt.length - sizeof(acpi_sdt); i++) {
        acpi_sdt *ptr;
        
        if (xsdt_found) {
            ptr = (acpi_sdt *)(((uint64_t *)rsdt->ptrs_start)[i] + VM_MEM_OFFSET);
        } else {
	        ptr = (acpi_sdt *)(((uint32_t *)rsdt->ptrs_start)[i] + VM_MEM_OFFSET);
        }

        if (memcmp(ptr->sig, signature, 4) == 0 && cnt++ == index) {
	        log("acpi: Found \"%s\" at %X\n", signature, ptr);
            return (void*)ptr;
        }
    }

    log("acpi: \"%s\" not found\n", signature);
    return NULL;
}

