kernel_src = files(
    'lib/fmt.c',
    'lib/bitops.c',
    'lib/builtins.c',

    'generic/init.c',
    'generic/log.c',
    'generic/fbcon.c',
    'generic/acpi/acpi.c',

    'vm/phys.c',
    'vm/vm.c',
)

kflags = ['-ffreestanding', '-fPIC', '-mgeneral-regs-only', '-nostdlib', '-fno-stack-protector']

ld_flags = ['-shared', '-pie', '-fPIC', '-nostdlib', '-Wl,--build-id=none', '-lgcc']

kernel_include_dirs = ['include']

if host_machine.cpu_family() == 'aarch64'
    subdir('arch/aarch64')
elif host_machine.cpu_family() == 'x86_64'
    subdir('arch/x86_64')
endif

executable(kernel_name, kernel_src, link_args: ld_flags, include_directories: include_directories(kernel_include_dirs), 
           c_args: kflags, install: true, install_dir: 'boot', link_depends: linker_file, pie: true)

