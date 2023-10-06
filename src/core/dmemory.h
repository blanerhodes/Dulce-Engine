#pragma once

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

