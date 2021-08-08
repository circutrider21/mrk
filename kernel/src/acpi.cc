#include <cstddef>
#include <internal/stivale2.h>
#include <klib/builtin.h>
#include <klib/vector.h>
#include <mrk/acpi.h>
#include <mrk/log.h>
#include <mrk/pmm.h>
#include <mrk/vmm.h>

static uint64_t revision = 0; // Assume ACPI 1.0 by default
static klib::vector<uint64_t>* acpi_tables;

static acpi::sdt_hdr* map_acpi_table(uintptr_t phys)
{
    auto* vmm = mm::vmm::kernel_space();
    vmm->map(phys, (phys + MEM_VIRT_OFFSET), PAGE_PRESENT | PAGE_NX, cache_type::nocache);
    auto* h = reinterpret_cast<acpi::sdt_hdr*>(phys + MEM_VIRT_OFFSET);

    uint64_t addr = phys - PAGE_SIZE;
    auto n_pages = ROUND_UP(h->length, PAGE_SIZE) + 2;
    for (uint64_t j = 0; j < n_pages; j++) {
        uint64_t phys = addr + (j * PAGE_SIZE);
        uint64_t virt = phys + MEM_VIRT_OFFSET;
        vmm->map(phys, virt, PAGE_PRESENT | PAGE_NX, cache_type::nocache);
    }

    return h;
}

namespace acpi {
void init()
{
    struct stivale2_struct_tag_rsdp* rk = (stivale2_struct_tag_rsdp*)arch::stivale2_get_tag(STIVALE2_STRUCT_TAG_RSDP_ID);
    if (rk->rsdp == 0) {
        PANIC("acpi: RSDP not found\n");
        return;
    }

    // Remap the RSDP as Read-Only and No-Execute
    mm::vmm::kernel_space()->unmap(rk->rsdp);
    mm::vmm::kernel_space()->map(rk->rsdp - MEM_VIRT_OFFSET, (uint64_t)rk->rsdp, PAGE_PRESENT | PAGE_NX, cache_type::nocache);

    auto* rsdp = (acpi::rsdp*)((uint64_t)rk->rsdp);
    revision = rsdp->revision;
    log("acpi: Revision -> %d\n", revision == 0 ? 1 : revision);

    rsdt* tables = nullptr;
    acpi_tables = new klib::vector<uint64_t>();

    if (revision > 1) {
        auto* xsdp = reinterpret_cast<acpi::xsdp*>(rsdp);
        tables = reinterpret_cast<acpi::rsdt*>(map_acpi_table(rsdp->rsdt_address));
    } else {
        tables = reinterpret_cast<acpi::rsdt*>(map_acpi_table(rsdp->rsdt_address));
    }

    char buf[5] = { 0 };
    buf[4] = 0;

    size_t entries = (tables->header.length - sizeof(acpi::sdt_hdr)) / ((revision > 1) ? 8 : 4);
    for (size_t i = 0; i < entries; i++) {
        uintptr_t entry = (revision > 1) ? (((acpi::xsdt*)tables)->tables[i]) : tables->tables[i];
        if (!entry)
            continue;

        acpi::sdt_hdr* h = map_acpi_table(entry);

        for (int g = 0; g < 4; g++)
            buf[g] = h->signature[g];

        log("acpi: Found Table \"%s\"\n", buf);
        acpi_tables->push_back(reinterpret_cast<uint64_t>(h));
    }
}

void* get_table(const char* signature, uint64_t index)
{
    uint64_t curr = 0;
    for (const auto table : (*acpi_tables)) {
        acpi::sdt_hdr* header = reinterpret_cast<acpi::sdt_hdr*>(table);

        if ((signature[0] == header->signature[0]) && (signature[1] == header->signature[1]) && (signature[2] == header->signature[2]) && (signature[3] == header->signature[3])) {
            if (curr != index) {
                curr++;
            } else {
                return (void*)header;
            }
        }
    }

    log("acpi: [WARN] Didn't find table %s\n", signature);
    return nullptr;
}
}
