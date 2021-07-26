#!/bin/sh -e

# bundle_iso.sh - builds a bootlable iso from a build dir
# PARAMETERS:
#    [1] - Build Dir that containes the compiled kernel

if [ ! -d $1 ] 
then
    echo "bundle_iso: build directory $1 does not exist."
    exit 1
fi

mkdir image-root

# Copy limine files
cp -n subprojects/limine-bin/limine.sys image-root/limine.sys
cp -n subprojects/limine-bin/limine-cd.bin image-root/limine-cd.bin
cp -n subprojects/limine-bin/limine-eltorito-efi.bin image-root/limine-efi.bin
mkdir -p image-root/EFI/BOOT && cp -n subprojects/limine-bin/BOOTX64.EFI image-root/EFI/BOOT/BOOTX64.EFI

# Copy the kernel
cp $1/mrk.elf image-root/mrk.elf
cp misc/limine.cfg image-root/limine.cfg

# Build the iso
xorriso -as mkisofs \
  -J -joliet-long \
  -rock \
  -b limine-cd.bin \
  -c limine-cd.cat \
  -no-emul-boot -boot-load-size 4 -boot-info-table \
  -eltorito-alt-boot \
  -e limine-efi.bin \
  -no-emul-boot -isohybrid-gpt-basdat \
  image-root \
  -o $1/mrk-image.iso

# Clean the iso output
rm -rf image-root

