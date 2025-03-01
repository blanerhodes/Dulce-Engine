@echo off
REM Build script for engine
SetLocal EnableDelayedExpansion

REM Get a list of all .cpp files
SET cFilenames=./src/win32_platform.cpp

REM echo "Files:" %cFilenames%
SET assembly=win32_dulce
SET compilerFlags=-Wvarargs -Wall -Werror -Wno-writable-strings -Wno-missing-braces -Wno-unused-function -Wno-microsoft-include -Wno-unused-variable -Wno-nonportable-include-path -Wno-class-conversion -Wno-uninitialized -Wno-unused-but-set-variable -Wno-address-of-temporary -Wno-bool-conversion -Wno-c++11-narrowing -Wc99-designator
SET includeFlags=-Isrc 
SET linkerFlags=-L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64" -luser32 -lgdi32 -ld3d12 -ldxgi -ldxguid -lwinmm -ld3dcompiler
SET defines=-DDEXPORT -D_CRT_SECURE_NO_WARNINGS

ECHO "Building debug %assembly%..."

clang++ -std=c++20 %cFilenames% -g %compilerFlags% -o ./bin/debug/%assembly%.exe -D_DEBUG %defines% %includeFlags% -lmsvcrtd %linkerFlags% 

ECHO Successfully built debug %assembly%

ECHO "Building release %assembly%..."

clang++ -std=c++20 %cFilenames% %compilerFlags% -o ./bin/release/%assembly%.exe %defines% %includeFlags% -lmsvcrt %linkerFlags% 

ECHO Successfully built release %assembly%