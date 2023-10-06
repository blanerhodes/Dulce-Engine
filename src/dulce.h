#pragma once

#include "platform_services.h"
#include "world.cpp"

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

struct LoadedBitmap {
    i32 width;
    i32 height;
    u32* pixels;
};

struct HeroBitmaps {
    i32 align_x;
    i32 align_y;
    LoadedBitmap head;
    LoadedBitmap cape;
    LoadedBitmap torso;
};

enum EntityType {
    ENTITY_TYPE_NULL, 
    ENTITY_TYPE_HERO, 
    ENTITY_TYPE_WALL
};


struct HighEntity {
    Vec2 pos; //already relative to the camera
    Vec2 velocity;
    u32 facing_direction;
    u32 chunk_z;
    f32 z;
    f32 delta_z;
    u32 low_entity_index;
};

struct LowEntity {
    EntityType type;
    WorldPosition pos;
    f32 height;
    f32 width;
    b32 collides;
    i32 dabs_tile_z;
    u32 high_entity_index;
};

struct Entity {
    u32 low_index;
    LowEntity* low;
    HighEntity* high;
};

struct LowEntityChunkRef {
    WorldChunk* tile_chunk;
    u32 entity_index_in_chunk;
};

struct GameState {
    MemoryArena world_arena;
    World* world;
    LoadedBitmap backdrop;
    LoadedBitmap shadow;
    HeroBitmaps hero_bitmaps[4];
    u32 camera_following_entity_index;
    WorldPosition camera_pos;
    u32 player_index_for_controller[ArrayCount(((GameInput*)0)->controllers)];

    u32 high_entity_count;
    HighEntity high_entities[256];
    u32 low_entity_count;
    LowEntity low_entities[100000];

    Camera camera;

    f32 t_sin;
};


DebugReadFileResult DebugPlatformReadEntireFile(ThreadContext* thread, char* filename);
void DebugPlatformFreeFileMemory(ThreadContext* thread, void* memory);
b32 DebugPlatformWriteEntireFile(ThreadContext* thread, char* filename, u32 memory_size, void* memory);

