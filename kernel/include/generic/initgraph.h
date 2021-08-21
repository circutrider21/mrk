#ifndef GENERIC_INITGRAPH_H
#define GENERIC_INITGRAPH_H

#include <stdint.h>

typedef struct {
    char* name;
    int pos;
    void (*func)();
    uint32_t flags;
} initgraph_t;

#define INITGRAPH_TARGET(nam, p, fun)\
__attribute__((section(".initgraph"), used))\
static initgraph_t nam = {\
    .name = #nam,\
    .pos = p,\
    .func = fun,\
};\

#define INITGRAPH_RAN (1 << 0)

void initgraph_dump();
void initgraph_run();

#endif // GENERIC_INITGRAPH_H

