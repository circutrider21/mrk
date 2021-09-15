kernel_src += files(
    'arch.c',
    'debug.c',
    'dtb.c',
    'vmm.c',
    'intr.c',
    'timer.c',
    'vectors.S',
)

kernel_name = 'mrk-aa64.elf'

kernel_include_dirs += ['arch/aarch64/include']

linker_file = meson.current_source_dir() + '/linker.ld'

ld_flags += ['-T', linker_file, '-maarch64elf', '-z', 'notext']

if win32 == true
  run_target('run-qemu',
    command : ['cmd', '/c',
      'qemu-system-aarch64 -M virt,iommu=smmuv3 -d int -cpu cortex-a72 -drive if=pflash,format=raw,file=assets/sabaton_virt_aa64.bin,readonly=on -m 4G -serial stdio -smp 4 -device ramfb -fw_cfg opt/Sabaton/kernel,file=chah\kernel\mrk-aa64.elf -device virtio-rng-pci'
    ])
else 
  run_target('run-qemu',
    command : ['sh', '-c',
      'qemu-system-aarch64 -M virt,iommu=smmuv3 -d int -cpu cortex-a72 -drive if=pflash,format=raw,file=${MESON_SOURCE_ROOT}/assets/sabaton_virt_aa64.bin,readonly=on -m 4G -serial stdio -smp 4 -device ramfb -fw_cfg opt/Sabaton/kernel,file=${MESON_BUILD_ROOT}/kernel/mrk-aa64.elf -device virtio-rng-pci'
    ])
endif