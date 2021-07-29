project('mrk', 'cpp', version: '0.1.0')

kernel_sources = files( 
    'arch/apic.cc',
    'arch/arch.cc',
    'arch/boot.cc',
    'arch/cpu.cc',
    'arch/gdt.cc',
    'arch/idt.cc',
    'arch/smp.cc',

    'mm/pmm.cc',
    'mm/vmm.cc',
    'mm/alloc.cc',
    'mm/mmap.cc',

    'fs/fs.cc',
    'fs/node.cc',
    'fs/tmpfs.cc',

    'kernel/acpi/acpi.cc',
    'kernel/proc/simd.cc',
    'kernel/log.cc',
    'kernel/lock.cc',
)

include_dirs = include_directories(['include', 'include/lib'])

kernel_asm_sources = files(
    'arch/intr.asm',
    'arch/funcs.asm',
    'kernel/proc/syscall.asm'
)

# Compile the asm files with a simple trick
nasm_cmd = find_program('nasm')
nasm_gen = generator(nasm_cmd, output: '@BASENAME@.o', arguments: ['-f', 'elf64', '-g', '-F', 'dwarf', '@INPUT@', '-o', '@OUTPUT@'])
nasm_objects = nasm_gen.process(kernel_asm_sources)

# Common flags shared across all C-family languages
flags_c_common = ['-m64', '-march=x86-64', '-fno-PIC', '-ffreestanding', '-mno-red-zone', 
                  '-nostdlib', '-mcmodel=kernel', '-fstack-protector', 
                  '-mno-mmx', '-mno-sse', '-mno-sse2', '-mno-sse3', '-mno-sse4', '-mno-sse4.1', '-mno-sse4.2', '-mno-sse4a']

if get_option('enable_ubsan')
    flags_c_common +=  ['-fsanitize=integer']
    kernel_sources += ['kernel/ubsan.cc']
endif

cpp_flags = flags_c_common
cpp_flags += ['-std=c++17', '-fno-exceptions', '-fno-rtti']

# Disable some pesky c++ warnings
cpp_flags += ['-Wno-inline-new-delete', '-Wno-write-strings']

linkscript = meson.current_source_dir() + '/misc/kernel.ld'
ld_flags = ['-T', linkscript, '-nostdlib', '-mcmodel=kernel', '-fno-PIC', '-no-pie', '-Wl,--build-id=none', 
            '-Wl,-z,max-page-size=0x1000,-n', '-lgcc']

add_global_arguments(cpp_flags, language: 'cpp')
add_global_link_arguments(ld_flags, language: 'cpp')

executable('mrk.elf', kernel_sources, nasm_objects, include_directories: include_dirs, install: true, link_depends: linkscript)

