#include <generic/log.h>
#include <generic/initgraph.h>
#include <arch/debug.h>
#include <arch/intr.h>

extern void __load_vectors();
INITGRAPH_TARGET(load_vectors, 1, &__load_vectors);

void handle_intr(struct arch_cpu_frame* trp, int el) {
    log("FATAL Exception, Dumping Registers...\n");
    debug_dump(trp, el);
    for(;;);
}

void handle_irq(struct arch_cpu_frame* trp, int el) {
    log("IRQ Recived!\n");
    debug_dump(trp, el);
    for(;;);
}

