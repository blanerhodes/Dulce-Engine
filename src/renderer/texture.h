#pragma once

//TODO: figure out some way to differentiate between the 128/512/1024 texture buffers
enum TextureID {
	TexID_Unset,
	TexID_Default,
	TexID_WhiteTexture,
	TexID_Sky,
	TexID_AwesomeFace,
	TexID_Sunrise
};

struct Texture {
	TextureID id;
	void* data;
};

//NOTE: this is set up more in line with Texture2DArray
enum TextureDim {
	TEXTURE_DIM_128 = 128,
	TEXTURE_DIM_512 = 512, 
	TEXTURE_DIM_1024 = 1024
};
#define MAX_TEXTURE_SLOTS 16
struct RendererTextureBuffer {
	u32 max_texture_slots;
	TextureDim dimension;
	Texture textures[MAX_TEXTURE_SLOTS];
};

struct TextureIdSrcPair {
	TextureID id;
	char src[20];
};
