#pragma once
#include "defines.h"

struct MemoryArena {
    u64 size;
    u8* base;
    u64 used;
};

struct Stack {
    u64 size;
    u8* base;
    u8* top;
    u8* last_push;
};

template<typename type>
struct CircularArray {
    u32 front;
    u32 back;
    u32 capacity; //NOTE: this must always be a power of 2!
    type* data;
};

template<typename type>
struct Queue {
    CircularArray<type> data;
};

struct PoolSlot {
    u8* data;
    u32 space_used;
};

struct Pool {
    u8* base_addr;
    u32 slot_size;
    u32 max_slots;
    u32 slots_used;
    PoolSlot* slots;
    Queue<u32> free_list;
};

struct PoolArena {
    MemoryArena storage;
    Pool pool;
};


void* PushSize_(MemoryArena* arena, size_t size);
void* PushCopy_(MemoryArena* arena, void* src, u64 size);


#define PushStruct(arena, type) (type*)PushSize_(arena, sizeof(type))
#define PushArray(arena, count, type) (type*)PushSize_(arena, (count) * sizeof(type))
#define PushSize(arena, size) (u8*)PushSize_(arena, size)
#define PushCopy(arena, src, count, type) (type*)PushCopy_(arena, src, (count) * sizeof(type))

#define PushStructStack(arena, type) (type*)PushSizeStack_(arena, sizeof(type))
#define PushArrayStack(arena, count, type) (type*)PushSizeStack_(arena, (count) * sizeof(type))
#define PushSizeStack(arena, size) (u8*)PushSizeStack_(arena, size)

void InitializeArena(MemoryArena* arena, u64 size, u8* base);
void ArenaZeroMemory(MemoryArena* arena);
void ArenaReset(MemoryArena* arena);
void MemCopy(void* src, void* dst, u64 size);