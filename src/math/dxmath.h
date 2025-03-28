#pragma once
#include "dmath.h"
#include "../defines.h"
#include <directxmath.h>

using namespace DirectX;

Mat4 DXMatToMat4(XMFLOAT4X4 mat) {
    Mat4 result = {};
    for (u32 i = 0; i < 4; i++) {
        for (u32 j = 0; j < 4; j++) {
            result.data[i*4 + j] = mat.m[i][j];
        }
    }
    return result;
}

Mat4 Mat4Perspective(f32 fov_degrees, f32 aspect_ratio, f32 near_clip = 1.0f, f32 far_clip = 1000.0f) {
    XMMATRIX p = XMMatrixPerspectiveFovRH(0.25*PI, aspect_ratio, near_clip, far_clip);
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, p);
    return DXMatToMat4(m);
}

Mat4 Mat4LookAt(Vec4 pos, Vec4 target, Vec4 up) {
    XMVECTOR xpos = XMVectorSet(pos.x, pos.y, pos.z, pos.w);
    XMVECTOR xtarget = XMVectorZero();
    XMVECTOR xup = XMVectorSet(up.x, up.y, up.z, up.w);
    XMMATRIX v = XMMatrixLookAtLH(xpos, xtarget, xup);
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, v);
    return DXMatToMat4(m);
}