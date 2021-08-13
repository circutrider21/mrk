#!/bin/sh 

# A Simple script that runs QEMU on a build directory
# PARAMETERS:
# [1] - Path to build dir

if [ -z $1 ] || [ -z $2 ]
then
    echo "USAGE: run-qemu.sh <build-dir> <source-dir>"
    exit 1
fi

if [ ! -d $1 ]
then
    meson setup $1
fi

if [ ! -d $2 ]
then
    echo "run-qemu: source dir $2 doesn't exist!"
    exit 1
fi

ninja -C $1
$2/tools/bundle_iso.sh $1 $2 &> /dev/null
qemu-system-x86_64 -smp 4 -d cpu_reset -D $1/log.txt --enable-kvm -cpu host,+invtsc -m 2G -M q35 -cdrom $1/mrk-image.iso -debugcon stdio -s

