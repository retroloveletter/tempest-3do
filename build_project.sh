#!/bin/sh

cd source
make clean
make || exit 1

cd ..
cp -f source/LaunchMe CD
3doiso.exe -in CD -out tempest3do.iso
3doEncrypt.exe genromtags tempest3do.iso

./launch.bat
