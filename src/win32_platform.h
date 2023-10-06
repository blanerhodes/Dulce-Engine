#pragma once

struct Win32FrameBuffer {
    BITMAPINFO info;
    void* memory;
    u32 width;
    u32 height;
    u32 pitch;
    i32 bytes_per_pixel;
};

struct Win32WindowDimensions {
    u32 width;
    u32 height;
};

struct Win32State {
    u64 total_size;
    void* game_memory_block;
    void* renderer_memory_block;
};
