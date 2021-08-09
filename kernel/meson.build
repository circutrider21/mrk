project('mrk', 'cpp', version: '0.1.0')

add_languages('c')

kernel_sources = files( 
    'src/arch/apic.cc',
    'src/arch/arch.cc',
    'src/arch/boot.cc',
    'src/arch/cpu.cc',
    'src/arch/gdt.cc',
    'src/arch/idt.cc',
    'src/arch/smp.cc',

    'src/mm/pmm.cc',
    'src/mm/vmm.cc',
    'src/mm/alloc.cc',
    'src/mm/mmap.cc',

    'src/proc/simd.cc',
    'src/proc/sched.cc',
    'src/acpi.cc',
    'src/log.cc',
    'src/lock.cc',

    'lib/klib/alloc.c',
    'lib/klib/builtin.cc',
)

include_dirs = include_directories(['include', 'include/lib'])

kernel_asm_sources = files(
    'src/arch/intr.asm',
    'src/arch/funcs.asm',
    'src/proc/syscall.asm'
)

# Compile the asm files with a simple trick
nasm_cmd = find_program('nasm')
nasm_gen = generator(nasm_cmd, output: '@BASENAME@.o', arguments: ['-f', 'elf64', '-g', '-F', 'dwarf', '@INPUT@', '-o', '@OUTPUT@'])
nasm_objects = nasm_gen.process(kernel_asm_sources)

# Common flags shared across all C-family languages
flags_c_common = ['-m64', '-march=x86-64', '-fno-PIC', '-ffreestanding', '-mno-red-zone', 
                  '-nostdlib', '-mcmodel=kernel', '-fno-stack-protector', 
                  '-mno-mmx', '-mno-sse', '-mno-sse2', '-mno-sse3', '-mno-sse4', '-mno-sse4.1', '-mno-sse4.2', '-mno-sse4a']

cpp_flags = flags_c_common
cpp_flags += ['-std=c++17', '-fno-exceptions', '-fno-rtti']

# Disable some pesky c++ warnings
cpp_flags += ['-Wno-inline-new-delete', '-Wno-write-strings']

linkscript = meson.current_source_dir() + '/../misc/kernel.ld'
ld_flags = ['-T', linkscript, '-nostdlib', '-mcmodel=kernel', '-fno-PIC', '-no-pie', '-Wl,--build-id=none', 
            '-Wl,-z,max-page-size=0x1000,-n', '-lgcc']

add_global_arguments(flags_c_common, language: 'c')

# UBSAN is only supported with GCC since Clang needs some extra things to get it working
message(meson.get_compiler('cpp').get_id())
if get_option('enable_ubsan') 
    flags_c_common +=  ['-fsanitize=undefined']
    kernel_sources += ['kernel/ubsan.cc']
endif

add_global_arguments(cpp_flags, language: 'cpp')
add_global_link_arguments(ld_flags, language: 'cpp')

executable('mrk.elf', kernel_sources, nasm_objects, include_directories: include_dirs, install: true, install_dir: 'boot', link_depends: linkscript)

