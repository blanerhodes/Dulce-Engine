#pragma once

#include "defines.h"
#include "camera.h"
#include "platform_services.h"


struct GameFrameBuffer {
    void* memory;
	i32 width;
    i32 height;
    i32 pitch;
    i32 bytes_per_pixel;
};

struct GameMemory {
    b32 is_initialized;
    u64 permanent_storage_size;
    void* permanent_storage;

    u64 transient_storage_size;
    void* transient_storage;

    //debug platform functions?
};

struct ThreadContext {
    i32 placeholder;
};

struct GameState {
    MemoryArena world_arena;
    u32 camera_following_entity_index;
    u32 player_index_for_controller[ArrayCount(((GameInput*)0)->controllers)];

    Camera camera;

    f32 t_sin;
};


DebugReadFileResult DebugPlatformReadEntireFile(ThreadContext* thread, char* filename);
void DebugPlatformFreeFileMemory(ThreadContext* thread, void* memory);
b32 DebugPlatformWriteEntireFile(ThreadContext* thread, char* filename, u32 memory_size, void* memory);

