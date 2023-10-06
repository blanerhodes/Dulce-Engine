#pragma once
//NOTE: this file is just forward declaring services that the platform provides to the engine

struct DebugReadFileResult {
    u32 contents_size;
    void* contents;
};
