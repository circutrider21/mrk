project('mrk', 'c', version: 'c', default_options: ['c_std=gnu11', 'warning_level=2', 'default_library=static'])

# Set the win32 variable (used throught the build files)

win32 = false

if build_machine.system() == 'windows'
    win32 = true
endif

# Build the documentation
subdir('docs')

# Build the kernel
subdir('kernel')
