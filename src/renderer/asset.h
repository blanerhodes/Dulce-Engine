#pragma once

struct AssetID {
	u8 name[32];
	u8 filepath[64];
};

struct AssetData {
	u8 name[32];
	Vertex* vertices;
	Index* indices;
	u32 ref_count;
};

struct AssetHashMap {
	AssetData* slots;
	u32 num_slots;
};

