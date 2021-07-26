lai_sources = files(
    'core/error.c',
    'core/eval.c',
    'core/exec.c',
    'core/exec-operand.c',
    'core/libc.c',
    'core/ns.c',
    'core/object.c',
    'core/opregion.c',
    'core/os_methods.c',
    'core/variable.c',
    'core/vsnprintf.c',
    'helpers/pc-bios.c',
    'helpers/pci.c',
    'helpers/resource.c',
    'helpers/sci.c',
    'helpers/pm.c',
    'drivers/ec.c',
    'drivers/timer.c',
)

lai_includes = include_directories('include')

liblai = static_library('lai', lai_sources,
    include_directories: lai_includes,
    pic: false)

lai_dep = declare_dependency(link_with: liblai,
    include_directories: lai_includes)

