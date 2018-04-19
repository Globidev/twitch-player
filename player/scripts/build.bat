@echo off
cd target
set VSCMD_START_DIR=%cd%
call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat" x64
set CL=/MP
nmake %1
