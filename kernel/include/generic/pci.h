#ifndef GENERIC_PCI_H
#define GENERIC_PCI_H

#include <generic/acpi.h>
#include <lib/list.h>

#define PCI_BAR_MMIO 3
#define PCI_BAR_PORT 4
#define PCI_BAR_INVL 5

#define PCI_CAP_DMA   (1 << 2) 
#define PCI_CAP_MMIO  (1 << 1)
#define PCI_CAP_PIO   (1 << 0)

struct __attribute__((packed)) allocation {
    uint64_t base;
    uint16_t segment;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
};

typedef struct __attribute__((packed)) {
    acpi_sdt header;
    uint64_t reserved;
    struct allocation allocs[];
} acpi_mcfg;

struct pci_bar {
    uint8_t type;
    uint64_t base;
    size_t len;
};

typedef struct pci_device {
    struct list_head hdr;

    uint16_t seg;
    uint8_t bus, slot, func;
    uintptr_t mmio_base;
} pci_dev_t;

void pci_init();

struct pci_bar pci_read_bar(pci_dev_t* dev, int bar);
void pci_set_priv(pci_dev_t* dev, uint8_t flags);

pci_dev_t* pci_get_dev(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func);
pci_dev_t* pci_get_by_class(uint8_t class_code, uint8_t subclass_code, uint8_t prog_if);

#endif // GENERIC_PCI_H
