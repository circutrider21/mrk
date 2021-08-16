kernel_src += files(
    'boot.c',
    'debug.c'
)

kernel_name = 'mrk-aa64.elf'

kernel_include_dirs += ['arch/aarch64/include']

linker_file = meson.current_source_dir() + '/linker.ld'

ld_flags += ['-T', linker_file, '-Wl,-maarch64elf', '-Wl,-z,notext']


run_target('run-qemu',
  command : ['sh', '-c',
		'qemu-system-aarch64 -M virt -cpu cortex-a57 -drive if=pflash,format=raw,file=${MESON_SOURCE_ROOT}/assets/sabaton_virt_aa64.bin,readonly=on -m 4G -serial stdio -smp 4 -device ramfb -fw_cfg opt/Sabaton/kernel,file=${MESON_BUILD_ROOT}/kernel/mrk-aa64.elf'
	])

