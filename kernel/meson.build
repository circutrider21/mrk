kernel_src = files(
    'lib/fmt.c',
    'lib/bitops.c',
    'lib/builtins.c',
    'lib/alloc.c',

    'generic/init.c',
    'generic/log.c',
    'generic/fbcon.c',
    'generic/acpi/acpi.c',

    'vm/phys.c',
    'vm/virtual.c',
    'vm/vm.c',
)

kflags = ['-ffreestanding', '-mgeneral-regs-only', '-nostdlib', '-fno-stack-protector']

ld_flags = ['-nostdlib', '--build-id=none']

kernel_include_dirs = ['include']

if host_machine.cpu_family() == 'aarch64'
    gnu_ld_command = 'aarch64-linux-gnu-ld'
    subdir('arch/aarch64')
elif host_machine.cpu_family() == 'x86_64'
    gnu_ld_command = 'ld'
    subdir('arch/x86_64')
endif

libkrnl = static_library('krnl', kernel_src, include_directories: include_directories(kernel_include_dirs), c_args: kflags, pic: true)

# As of right now, we only support 2 linkers which are either GNU ld and LLVM lld
if meson.get_compiler('c').get_id() == 'clang'
    custom_target('kernel',
            input: [libkrnl],
            output: [kernel_name],
            command: ['ld.lld', '-o', '@OUTPUT@', ld_flags, '@INPUT@'],
            install: true, install_dir: 'boot')
else
    custom_target('kernel',
            input: [libkrnl],
            output: [kernel_name],
            command: [gnu_ld_command, '-o', '@OUTPUT@', ld_flags, '@INPUT@'],
            install: true, install_dir: 'boot')
endif
