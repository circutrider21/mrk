ENTRY(mrk_entry)
OUTPUT_FORMAT(elf64-x86-64) 

SECTIONS 
{
    . = 0xFFFFFFFF80200000;

    __kernel_start = .;

    .stivale2hdr : ALIGN(4K) {
        KEEP(*(.stivale2hdr))
    }

    . = ALIGN(4K);
    .text : {
        __text_start = .;
        *(.text .text.*)
        . = ALIGN(4096);
        __text_end = .;
    }

    . = ALIGN(4K);
    .rodata : {
        __rodata_start = .;
        *(.rodata*)
        . = ALIGN(4096);
        __rodata_end = .;
    }

    /* The tdata section is used by the rust runtime for CPU local storage... */
    . = ALIGN(4K);
    .tdata : {
        __tdata_start = .;
        KEEP(*(.tdata*))
        KEEP(*(.tbss*))
        __tdata_end = ALIGN(8);
    }

    . = ALIGN(4K);
    .data : {
        __data_start = .;
        *(.data .data.*)
        . = ALIGN(4096);
        __data_end = .;
    }

    . = ALIGN(4K);
    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        . = ALIGN(4096);
        __bss_end = .;
    }
    
    __kernel_end = .;
}
