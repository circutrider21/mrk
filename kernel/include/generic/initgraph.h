#ifndef GENERIC_INITGRAPH_H
#define GENERIC_INITGRAPH_H

#include <stdint.h>

typedef struct {
    char* name;
    int pos;
    void (*func)();
} initgraph_t;

#define INITGRAPH_TARGET(nam, p, fun)\
__attribute__((section(".initgraph"), used))\
static initgraph_t nam = {\
    .name = #nam,\
    .pos = p,\
    .func = fun,\
};\

void initgraph_dump();

#endif // GENERIC_INITGRAPH_H

