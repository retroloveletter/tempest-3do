@echo off

set exe_path=C:\RetroArch-Win64
set core_path=C:\RetroArch-Win64\cores
set core=opera_libretro.dll

start %exe_path%\retroarch.exe -L %core_path%\%core% ./tempest3do.iso --verbose
