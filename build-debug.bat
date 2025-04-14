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

REM SET bin_file=phongVS.cso
REM SET src_file=phongVS.hlsl
SET shader_bin_dir=.\bin\debug\shaders\
SET shader_src_dir=.\resources\shaders\

REM SET /a src_newer=0
REM xcopy /L /D /Y %shader_src_dir%%src_file% %shader_bin_dir%%bin_file%|findstr /B /C:"1 " >nul && set /a src_newer=1 >nul


REM ECHO Before loop
REM FOR /F %%i IN ("%shader_src_dir%*.hlsl") do (
REM     ECHO In loop
REM     SET file_name=%%~ni
REM     xcopy /L /D /Y %shader_src_dir%!file_name!.hlsl %shader_bin_dir%!file_name!.cso|findstr /B /C:"1 " >nul && (
REM         ECHO SRC is newer
REM         ECHO.!src_file! | findstr /C:"VS" 1>nul
REM         if errorlevel 0 (
REM             ECHO SRC is pixel
REM             fxc /Od /Zi /Fd %shader_bin_dir%!file_name!.pdb /T ps_5_0 /Fo %shader_bin_dir%!file_name!.cso %shader_src_dir%!file_name!.hlsl
REM         ) else (
REM             ECHO SRC is vertex
REM             fxc /Od /Zi /Fd %shader_bin_dir%!file_name!.pdb /T vs_5_0 /Fo %shader_bin_dir%!file_name!.cso %shader_src_dir%!file_name!.hlsl
REM         )
REM     )
REM )
 
REM fxc /Od /Zi /Fd ./bin/debug/shaders/textured_vertex.pdb /T vs_5_0 /Fo ./bin/debug/shaders/textured_vertex.cso ./resources/shaders/textured_vertex.hlsl
REM fxc /Od /Zi /Fd ./bin/debug/shaders/textured_pixel.pdb /T ps_5_0 /Fo ./bin/debug/shaders/textured_pixel.cso ./resources/shaders/textured_pixel.hlsl

fxc /Od /Zi /Fd ./bin/debug/shaders/phongVS.pdb /T vs_5_0 /Fo ./bin/debug/shaders/phongVS.cso ./resources/shaders/phongVS.hlsl
fxc /Od /Zi /Fd ./bin/debug/shaders/phongPS.pdb /T ps_5_0 /Fo ./bin/debug/shaders/phongPS.cso ./resources/shaders/phongPS.hlsl

fxc /Od /Zi /Fd ./bin/debug/shaders/textured_phongPS.pdb /T ps_5_0 /Fo ./bin/debug/shaders/textured_phongPS.cso ./resources/shaders/textured_phongPS.hlsl

 
ECHO Successfully built shaders