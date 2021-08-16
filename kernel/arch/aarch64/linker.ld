ENTRY(mrk_start_aa64)

PHDRS {
  none   PT_NULL FLAGS(0);
  text   PT_LOAD FLAGS((1 << 2) | (1 << 0)  /* Readable | Executable */);
  rodata PT_LOAD FLAGS((1 << 2)             /* Readable */);
  data   PT_LOAD FLAGS((1 << 2) | (1 << 1)  /* Readable | Writeable  */);
}

/* For aarch64, it is required that the kernel is aligned on 64KB bounds
 * Whereas on x86-64, bounds are only 4KB */

SECTIONS {
  . = 0xFFFFFFFF80200000;

  . = ALIGN(64K);
  .text . : {
    *(.text*)
  }:text
  
  . = ALIGN(64K);
  .rodata . : {
    *(.rodata*)
  }:rodata
  
  . = ALIGN(64K);
  .stivale2hdr : {
    KEEP(*(.stivale2hdr))
  }:rodata

  . = ALIGN(64K);
  .initgraph : {
    __initgraph_start = .;
    KEEP(*(.initgraph)) 
    __initgraph_end = .;
  }:rodata

  . = ALIGN(64K);
  .data . : {
    *(.data*)
  }:data

  . = ALIGN(64K);
  .bss . : {
    __stack_low = .;
    . += 16K;
    __stack_high = .;
    *(.bss*)
  }
}

