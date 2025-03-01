@echo off
REM Build script for engine
SetLocal EnableDelayedExpansion

REM Get a list of all .cpp files
SET cFilenames=./src/win32_platform.cpp

REM echo "Files:" %cFilenames%
SET assembly=win32_dulce
SET compilerFlags=-Wvarargs -Wall -Werror -Wno-writable-strings -Wno-missing-braces -Wno-unused-function -Wno-microsoft-include -Wno-unused-variable -Wno-nonportable-include-path -Wno-class-conversion -Wno-uninitialized -Wno-unused-but-set-variable -Wno-address-of-temporary -Wno-bool-conversion -Wno-c++11-narrowing -Wc99-designator
SET includeFlags=-Isrc 
SET linkerFlags=-lmsvcrt -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64" -luser32 -lgdi32 -ld3d12 -ldxgi -ldxguid -lwinmm -ld3dcompiler
SET defines=-DDEXPORT -D_CRT_SECURE_NO_WARNINGS

ECHO "Building release %assembly%..."

clang++ -std=c++20 %cFilenames% %compilerFlags% -o ./bin/release/%assembly%.exe %defines% %includeFlags% %linkerFlags% 

ECHO Successfully built release %assembly%

 ECHO Building shaders...
 
 dxc /T rootsig_1_1 /E ROOTSIG ./shaders/RootSignature.hlsl -rootsig-define ROOTSIG /Fo ./bin/release/shaders/RootSignature.cso
 dxc /T vs_6_0 /Fo ./bin/release/shaders/VertexShader.cso ./shaders/VertexShader.hlsl
 dxc /T ps_6_0 /Fo ./bin/release/shaders/PixelShader.cso ./shaders/PixelShader.hlsl
 
 ECHO Successfully built shaders