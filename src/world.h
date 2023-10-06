#pragma once

struct WorldPosition {
    i32 chunk_x;
    i32 chunk_y;
    i32 chunk_z;
    //offset from chunk center
    Vec2 offset_;
};

struct WorldDifference {
    Vec2 delta_xy;
    f32 delta_z;
};


struct WorldEntityBlock {
    u32 entity_count;
    u32 low_entity_indices[16];
    WorldEntityBlock* next;
};

struct WorldChunk {
    i32 chunk_x;
    i32 chunk_y;
    i32 chunk_z;

    WorldEntityBlock first_block;
    WorldChunk* next_in_hash;
};

struct World {
    f32 tile_side_in_meters;
    f32 chunk_side_in_meters;

    WorldChunk world_chunk_hash[4096];
    WorldEntityBlock* first_free;
};
