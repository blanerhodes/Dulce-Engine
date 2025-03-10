#include "dmemory.h"
#include "defines.h"
#include "asserts.h"

void* PushCopy_(MemoryArena* arena, void* src, u64 size) {
    DASSERT((arena->used + size) <= arena->size);
    void* result = arena->base + arena->used;
    memcpy(result, src, size);
    arena->used += size;

    return result;
}

void* PushSize_(MemoryArena* arena, size_t size) {
    DASSERT((arena->used + size) <= arena->size);
    void* result = arena->base + arena->used;
    arena->used += size;

    return result;
}

void InitializeArena(MemoryArena* arena, u64 size, u8* base) {
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}

void ArenaZeroMemory(MemoryArena* arena) {
    memset(arena->base, 0, arena->size);
}

void ArenaReset(MemoryArena* arena) {
    ArenaZeroMemory(arena);
    arena->used = 0;
}

void MemCopy(void* src, void* dst, u64 size) {
    memcpy(dst, src, size);
}

template <typename T>
static u32 CircularArrayMask(CircularArray<T>* array, u32 val) {
    u32 result = val & (array->capacity - 1);
    return result;
}

template <typename T>
static u32 CircularArraySize(CircularArray<T>* array) {
    return array->back - array->front;
}

template<typename T>
static b32 CircularArrayIsFull(CircularArray<T>* array) {
    return CircularArraySize(array) == array->capacity;
}

template<typename T>
static b32 CircularArrayIsEmpty(CircularArray<T>* array) {
    return array->front == array->back;
}

static b32 IsPowOf2(u32 val) {
    b32 result = (val != 1) && !(val & (val-1));
    return result;
}

//NOTE: maybe make a version later on that can take any size and work the original way or expand to a power of 2 for the user
template<typename T>
static void CircularArrayInit(MemoryArena* arena, CircularArray<T>* array, u32 count) {
    DASSERT(count > 0);
    DASSERT(IsPowOf2(count));
    array->capacity = count;
    array->front = 0;
    array->back = 0;
    array->data = PushArray(arena, count, T);
}


//NOTE(spader HMN): When you push, instead of memcpying the object, 
//                   it's better to just assign (i.e. array->data[array->back] = T)
template<typename T>
static void CircularArrayPush(CircularArray<T>* array, T object) {
    DASSERT(!CircularArrayIsFull(array));
    MemCopy(&object, array->data+CircularArrayMask(array, array->back), sizeof(T));
    array->back++;
}

template<typename T>
static T CircularArrayPop(CircularArray<T>* array) {
    DASSERT(!CircularArrayIsEmpty(array));
    T result = *(array->data + CircularArrayMask(array, array->front));
    array->front++;
    return result;
}

template<typename T>
static b32 QueueIsFull(Queue<T>* queue) {
    return CircularArrayIsFull(&queue->data);
}

template<typename T>
static b32 QueueIsEmpty(Queue<T>* queue) {
    return CircularArrayIsEmpty(&queue->data);
}

template<typename T>
static void QueueInit(MemoryArena* arena, Queue<T>* queue, u32 count) {
    DASSERT(count > 0);
    CircularArrayInit(arena, &queue->data, count);
}

template<typename T>
static void QueuePush(Queue<T>* queue, T object) {
    DASSERT(!QueueIsFull(queue));
    CircularArrayPush(&queue->data, object);
}

template<typename T>
static T QueuePop(Queue<T>* queue) {
    DASSERT(!QueueIsEmpty(queue));
    return CircularArrayPop(&queue->data);
}

static void PoolInitWithBackBuffer(u8* pool_buffer, MemoryArena* arena, Pool* pool, u32 num_slots, u32 slot_size) {
    pool->slots = PushArray(arena, num_slots, PoolSlot);
    for (u32 slot = 0; slot < num_slots; slot++) {
        *(pool->slots + slot) = {
            .data = pool->base_addr + slot * pool->slot_size,
            .space_used = 0
        };
    }
    QueueInit(arena, &pool->free_list, num_slots);
    for (u32 i = 0; i < num_slots; i++) {
        QueuePush(&pool->free_list, i);
    }
}

static void PoolInit(MemoryArena* arena, Pool* pool, u32 num_slots, u32 slot_size) {
    pool->base_addr = PushSize(arena, slot_size*num_slots); 
    PoolInitWithBackBuffer(pool->base_addr, arena, pool, num_slots, slot_size);
}

static PoolSlot* PoolGetSlot(Pool* pool, u32 slot) {
    return &pool->slots[slot];
}

static void PoolPushWithinSlot(Pool* pool, void* data, u32 slot, u32 size, b32 overwrite = false) {
    DASSERT(slot < pool->max_slots);
    if (overwrite) {
        MemCopy(data, pool->slots[slot].data, pool->slot_size);
    }
    MemCopy(data, pool->slots[slot].data, pool->slot_size);
    pool->slots_used++;
}

static void PoolPushToSlot(Pool* pool, void* data, u32 slot) {
    DASSERT(slot < pool->max_slots);
    MemCopy(data, pool->slots[slot].data, pool->slot_size);
    pool->slots_used++;
}

static void PoolPush(Pool* pool, void* data) {
    u32 free_slot = QueuePop(&pool->free_list);
    PoolPushToSlot(pool, data, free_slot);
}


static void InitializeArena(PoolArena* arena, u32 num_slots, u32 slot_size, u8* base) {
    arena->pool.max_slots = num_slots;
    arena->pool.slot_size = slot_size;
    u32 memory_for_slot_buffer = num_slots * slot_size;
    u32 memory_for_pool_slots = num_slots * sizeof(PoolSlot);
    u32 total_memory_needed = memory_for_slot_buffer + memory_for_pool_slots;

    InitializeArena(&arena->storage, total_memory_needed, base);
    PoolInit(&arena->storage, &arena->pool, num_slots, slot_size);
}

