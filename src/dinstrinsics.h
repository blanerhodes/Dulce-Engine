#pragma once
#include "defines.h"

struct BitScanResult {
    b32 found;
    u32 index;
};

static BitScanResult FindSetLSB(u32 value) {
    BitScanResult result = {};
    result.found = _BitScanForward((unsigned long*)&result.index, value);

#if 0
    for (u32 test = 0; test < 32; test++) {
        if (value & (1 << test)) {
            result.index = test;
            result.found = true;
            break;
        }
    }
#endif
    return result;
}
