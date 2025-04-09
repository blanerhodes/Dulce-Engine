@echo off
REM Build script for engine
SetLocal EnableDelayedExpansion

REM Get a list of all .cpp files
SET cFilenames=./src/win32_platform.cpp

REM echo "Files:" %cFilenames%
SET assembly=win32_dulce
SET compilerFlags=-g -Wvarargs -Wall -Werror -Wno-writable-strings -Wno-missing-braces -Wno-unused-function -Wno-microsoft-include -Wno-unused-variable -Wno-nonportable-include-path -Wno-class-conversion -Wno-uninitialized -Wno-unused-but-set-variable -Wno-address-of-temporary -Wno-bool-conversion -Wno-c++11-narrowing -Wc99-designator
SET includeFlags=-Isrc -Isrc/core -Isrc/math -I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um"
SET linkerFlags=-lmsvcrtd -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64" -luser32 -lgdi32 -ld3d11 -ldxgi -ldxguid -lwinmm -ld3dcompiler
SET defines=-D_DEBUG -DDEXPORT -D_CRT_SECURE_NO_WARNINGS

ECHO "Building debug %assembly%..."

clang++ -std=c++20 %cFilenames% %compilerFlags% -o ./bin/debug/%assembly%.exe %defines% %includeFlags% %linkerFlags% 

ECHO Successfully built debug %assembly%

ECHO Building shaders...
 
fxc /Od /Zi /Fd ./bin/debug/shaders/textured_vertex.pdb /T vs_5_0 /Fo ./bin/debug/shaders/textured_vertex.cso ./resources/shaders/textured_vertex.hlsl
fxc /Od /Zi /Fd ./bin/debug/shaders/textured_pixel.pdb /T ps_5_0 /Fo ./bin/debug/shaders/textured_pixel.cso ./resources/shaders/textured_pixel.hlsl

fxc /Od /Zi /Fd ./bin/debug/shaders/phongVS.pdb /T vs_5_0 /Fo ./bin/debug/shaders/phongVS.cso ./resources/shaders/phongVS.hlsl
fxc /Od /Zi /Fd ./bin/debug/shaders/phongPS.pdb /T ps_5_0 /Fo ./bin/debug/shaders/phongPS.cso ./resources/shaders/phongPS.hlsl

REM fxc /Od /Zi /Fd ./bin/debug/shaders/untextured_vertex.pdb /T vs_5_0 /Fo ./bin/debug/shaders/untextured_vertex.cso ./resources/shaders/untextured_vertex.hlsl
fxc /Od /Zi /Fd ./bin/debug/shaders/untextured_pixel.pdb /T ps_5_0 /Fo ./bin/debug/shaders/untextured_pixel.cso ./resources/shaders/untextured_pixel.hlsl
 
ECHO Successfully built shaders