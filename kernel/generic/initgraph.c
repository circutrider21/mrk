#include <generic/initgraph.h>
#include <generic/log.h>

extern initgraph_t __initgraph_start[];
extern initgraph_t __initgraph_end[];

static initgraph_t initgraph_get(int index) {
    uint64_t targets = ((uint64_t)__initgraph_end - (uint64_t)__initgraph_start) / sizeof(initgraph_t);
    for(uint64_t i = 0; i < targets; i++) {
        initgraph_t cur_target = __initgraph_start[i];
	if(cur_target.pos == index)
	    return cur_target;
    }

    return (initgraph_t){ .pos = -1 };
}

void initgraph_dump() {
    uint64_t targets = ((uint64_t)__initgraph_end - (uint64_t)__initgraph_start) / sizeof(initgraph_t);
    log("initgraph: A total of %d targets, dumping order...\n", targets);

    for(uint64_t i = 0; i < targets; i++) {
        initgraph_t cur_target = initgraph_get(i);
	log("\t%s -> %d\n", cur_target.name, cur_target.pos);
    }
}

