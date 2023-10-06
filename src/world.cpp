#include "world.h"
#include "asserts.h"

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64) 
#define TILE_CHUNK_UNINITIALIZED INT32_MAX
#define TILES_PER_CHUNK 16

static WorldChunk* GetWorldChunk(World* world, i32 tile_chunk_x, i32 tile_chunk_y, i32 tile_chunk_z, MemoryArena* arena = 0) {
    DASSERT(InRangeOpenI(tile_chunk_x, -TILE_CHUNK_SAFE_MARGIN, TILE_CHUNK_SAFE_MARGIN));
    DASSERT(InRangeOpenI(tile_chunk_y, -TILE_CHUNK_SAFE_MARGIN, TILE_CHUNK_SAFE_MARGIN));
    DASSERT(InRangeOpenI(tile_chunk_z, -TILE_CHUNK_SAFE_MARGIN, TILE_CHUNK_SAFE_MARGIN));

    u32 hash_value = 19*tile_chunk_x + 7*tile_chunk_y + 3*tile_chunk_z;
    u32 hash_slot = hash_value & (ArrayCount(world->world_chunk_hash)-1);
    DASSERT(hash_slot < ArrayCount(world->world_chunk_hash));

    WorldChunk* chunk = world->world_chunk_hash + hash_slot;
    do {
        if (tile_chunk_x == chunk->chunk_x && tile_chunk_y == chunk->chunk_y && tile_chunk_z == chunk->chunk_z) {
            break;
        }

        if (arena && chunk->chunk_x != TILE_CHUNK_UNINITIALIZED && !chunk->next_in_hash) {
            chunk->next_in_hash = PushStruct(arena, WorldChunk);
            chunk = chunk->next_in_hash;
            chunk->chunk_x = TILE_CHUNK_UNINITIALIZED;
        }

        if (arena && chunk->chunk_x == TILE_CHUNK_UNINITIALIZED) {
            chunk->chunk_x = tile_chunk_x;
            chunk->chunk_y = tile_chunk_y;
            chunk->chunk_z = tile_chunk_z;

            chunk->next_in_hash = 0;
            break;
        }
        chunk = chunk->next_in_hash;

    } while (chunk);

    return chunk;
}

inline b32 IsCoordCorrected(World* world, f32 tile_rel) {
    b32 result = (tile_rel >= -0.5f * world->chunk_side_in_meters) &&
                 (tile_rel <=  0.5f * world->chunk_side_in_meters);
    return result;
}

inline b32 IsCoordCorrected(World* world, Vec2 offset) {
    b32 result = IsCoordCorrected(world, offset.x) && IsCoordCorrected(world, offset.y);
    return result;
}

inline void GetCorrectedCoord(World* world, i32* tile, f32* tile_rel) {
    i32 offset = RoundF32ToI32(*tile_rel / world->chunk_side_in_meters);
    *tile += offset;
    *tile_rel -= offset * world->chunk_side_in_meters;

    DASSERT(IsCoordCorrected(world, *tile_rel));
}

inline WorldPosition MapIntoTileSpace(World* world, WorldPosition base_pos, Vec2 offset) {
    WorldPosition result = base_pos;
  
    result.offset_ += offset;
    GetCorrectedCoord(world, &result.chunk_x, &result.offset_.x);
    GetCorrectedCoord(world, &result.chunk_y, &result.offset_.y);

    return result;
}

static b32 AreInSameChunk(World* world, WorldPosition* a, WorldPosition* b) {
    DASSERT(IsCoordCorrected(world, a->offset_));
    DASSERT(IsCoordCorrected(world, b->offset_));
    b32 result = a->chunk_x == b->chunk_x && a->chunk_y == b->chunk_y && a->chunk_z == b->chunk_z;
    return result;
}

inline WorldPosition ChunkPosFromTilePos(World* world, i32 abs_tile_x, i32 abs_tile_y, i32 abs_tile_z) {
    WorldPosition result = {};
    result.chunk_x = abs_tile_x / TILES_PER_CHUNK;
    result.chunk_y = abs_tile_y / TILES_PER_CHUNK;
    result.chunk_z = abs_tile_z / TILES_PER_CHUNK;
    result.offset_.x = (f32)(abs_tile_x - (result.chunk_x*TILES_PER_CHUNK)) * world->tile_side_in_meters;
    result.offset_.y = (f32)(abs_tile_y - (result.chunk_y*TILES_PER_CHUNK)) * world->tile_side_in_meters;
    return result;
}

static WorldDifference Subtract(World* world, WorldPosition* a, WorldPosition* b) {
    WorldDifference result = {};

    Vec2 delta_tile_xy = { (f32)a->chunk_x - b->chunk_x, (f32)a->chunk_y - b->chunk_y };
    f32 delta_tile_z = (f32)a->chunk_z - b->chunk_z;

    result.delta_xy = world->chunk_side_in_meters * delta_tile_xy + (a->offset_ - b->offset_);
    result.delta_z = world->chunk_side_in_meters * delta_tile_z;

    return result;
}

static WorldPosition CenteredChunkPoint(u32 chunk_x, u32 chunk_y, u32 chunk_z) {
    WorldPosition result = {};
    result.chunk_x = chunk_x;
    result.chunk_y = chunk_y;
    result.chunk_z = chunk_z;

    return result;
}

static void InitWorld(World* world, f32 tile_side_in_meters) {
    world->tile_side_in_meters = tile_side_in_meters;
    world->chunk_side_in_meters = (f32)TILES_PER_CHUNK * tile_side_in_meters;
    world->first_free = 0;

    for (u32 chunk_index = 0; chunk_index < ArrayCount(world->world_chunk_hash); chunk_index++) {
        world->world_chunk_hash[chunk_index].chunk_x = TILE_CHUNK_UNINITIALIZED;
        world->world_chunk_hash[chunk_index].first_block.entity_count = 0;
    }
}

inline void ChangeEntityLocation(MemoryArena* arena, World* world, u32 low_entity_index, WorldPosition* old_pos, WorldPosition* new_pos) {
    if (old_pos && AreInSameChunk(world, old_pos, new_pos)) {

    }
    else {
        if (old_pos) {
            WorldChunk* chunk = GetWorldChunk(world, old_pos->chunk_x, old_pos->chunk_y, old_pos->chunk_z, arena);
            DASSERT(chunk);
            if (chunk) {
                WorldEntityBlock* first_block = &chunk->first_block;
                for (WorldEntityBlock* block = first_block; block; block = block->next) {
                    for (u32 index = 0; index < block->entity_count; index++) {
                        if (block->low_entity_indices[index] == low_entity_index) {
                            block->low_entity_indices[index] = first_block->low_entity_indices[--first_block->entity_count];
                            if (first_block->entity_count == 0) {
                                if (first_block->next) {
                                    WorldEntityBlock* next_block = first_block->next;
                                    *first_block = *next_block;
                                    next_block->next = world->first_free;
                                    world->first_free = next_block;
                                }
                            }
                        }
                        block = 0;
                        break;
                    }
                }
            }
        }
        WorldChunk* chunk = GetWorldChunk(world, new_pos->chunk_x, new_pos->chunk_y, new_pos->chunk_z, arena);
        WorldEntityBlock* block = &chunk->first_block;
        if (block->entity_count == ArrayCount(block->low_entity_indices)) {
            WorldEntityBlock* old_block = world->first_free;
            if (old_block) {
                world->first_free = old_block->next;
            }
            else {
                old_block = PushStruct(arena, WorldEntityBlock);
            }
            *old_block = *block;
            block->next = old_block;
            block->entity_count = 0;
        }
        DASSERT(block->entity_count < ArrayCount(block->low_entity_indices));
        block->low_entity_indices[block->entity_count++] = low_entity_index;
    }
}

#if 0
static u32 GetTileValueUnchecked(TileMap* world, TileChunk* tile_chunk, i32 tile_x, i32 tile_y) {
    DASSERT(tile_chunk);
    DASSERT(tile_x < world->chunk_dimension);
    DASSERT(tile_y < world->chunk_dimension);
    u32 tile_chunk_value = tile_chunk->tiles[tile_y * world->chunk_dimension + tile_x];
    return tile_chunk_value;
}

static u32 GetTileValue(TileMap* world, TileChunk* tile_chunk, i32 tile_x, i32 tile_y) {
    u32 tile_chunk_value = 0;
    if (tile_chunk && tile_chunk->tiles) {
        tile_chunk_value = tile_chunk->tiles[tile_y * world->chunk_dimension + tile_x];
    }
    return tile_chunk_value;
}

static u32 GetTileValue(TileMap* world, u32 abs_tile_x, u32 abs_tile_y, u32 abs_tile_z) {
    TileChunkPosition chunk_pos = GetChunkPosition(world, abs_tile_x, abs_tile_y, abs_tile_z);
    TileChunk* tile_chunk = GetTileChunk(world, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);
    u32 tile_chunk_value = GetTileValue(world, tile_chunk, chunk_pos.rel_tile_x, chunk_pos.rel_tile_y);
    return tile_chunk_value;
}

static u32 GetTileValue(TileMap* world, TileMapPosition pos) {
    u32 result = GetTileValue(world, pos.abs_tile_x, pos.abs_tile_y, pos.abs_tile_z);
    return result;
}


static void SetTileValue(TileMap* world, TileChunk* tile_chunk, i32 tile_x, i32 tile_y, u32 tile_value) {
    DASSERT(tile_chunk);
    DASSERT(tile_x < world->chunk_dimension);
    DASSERT(tile_y < world->chunk_dimension);
    tile_chunk->tiles[tile_y * world->chunk_dimension + tile_x] = tile_value;
}

static void SetTileValue(MemoryArena* arena, TileMap* world, u32 abs_tile_x, u32 abs_tile_y, u32 abs_tile_z, u32 tile_value) {
    TileChunkPosition chunk_pos = GetChunkPosition(world, abs_tile_x, abs_tile_y, abs_tile_z);
    TileChunk* tile_chunk = GetTileChunk(world, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z, arena);
    SetTileValue(world, tile_chunk, chunk_pos.rel_tile_x, chunk_pos.rel_tile_y, tile_value);
}

static b32 IsTileValueEmpty(u32 tile_chunk_value) {
    b32 is_valid = tile_chunk_value == 1 || tile_chunk_value == 3 || tile_chunk_value == 4;
    return is_valid;
}

static b32 IsValidTilePosition(TileMap* world, TileChunk* tile_chunk, u32 test_tile_x, u32 test_tile_y) {
    bool is_valid = false;
    if (tile_chunk) {
        u32 tile_chunk_value = GetTileValue(world, tile_chunk, test_tile_x, test_tile_y);
        is_valid = tile_chunk_value == 0;
    }
    return is_valid;
}

static b32 IsValidTileMapPointPosition(TileMap* world, TileMapPosition position) {
    u32 tile_chunk_value = GetTileValue(world, position.abs_tile_x, position.abs_tile_y, position.abs_tile_z);
    b32 is_valid = IsTileValueEmpty(tile_chunk_value);
    return is_valid;
}
#endif

