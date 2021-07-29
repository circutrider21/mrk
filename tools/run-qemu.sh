#!/bin/sh 

# A Simple script that runs QEMU on a build directory
# PARAMETERS:
# [1] - Path to build dir

if [ -z $1 ]
then
    echo "USAGE: run-qemu.sh <build dir>"
    exit 1
fi

if [ ! -d $1 ]
then
    meson setup $1
    exit 1
fi

ninja -C $1 &> /dev/null
./tools/bundle_iso.sh $1
qemu-system-x86_64 -m 2G -M q35 -cdrom $1/mrk-image.iso -debugcon stdio -s


