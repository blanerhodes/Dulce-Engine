@echo off
REM Build script for shaders
SetLocal EnableDelayedExpansion

ECHO Building shaders...
 
REM dxc /T rootsig_1_1 /E ROOTSIG ./passes/example/RootSignature.hlsl -rootsig-define ROOTSIG /Fo ./bin/RootSignature.cso
dxc /Od /Zi /Fd ./bin/debug/vertex_debug.pdb /T vs_6_0 /Fo ./bin/debug/VertexShader.cso ./shaders/VertexShader.hlsl
REM dxc /Od /Zi /Fd ./bin/pixel_shader.pdb /T ps_6_0 /Fo ./bin/PixelShader.cso ./passes/example/PixelShader.hlsl
 
ECHO Successfully built shaders