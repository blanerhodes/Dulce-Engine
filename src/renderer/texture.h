#pragma once

//TODO: figure out some way to differentiate between the 128/512/1024 texture buffers
enum TextureID {
	TexID_Unset,
	TexID_Default,
	TexID_WhiteTexture,
	TexID_Sky,
	TexID_AwesomeFace,

	//backgrounds
	TexID_Sunrise,
	TexID_MountainBackground,
	TexID_Library,
	TexID_FlorenceNight,

	//winnie/family
	TexID_Baby,
	TexID_Powerlifting,
	TexID_Tractor,
	TexID_WinnieHouse,
	TexID_AshWedding,
	TexID_LunaTed,
	TexID_Honkem,

	//outdoors
	TexID_Hike,
	TexID_Skydive,
	TexID_Ouray,
	TexID_Proposal,

	//italy
	TexID_Basillica,
	TexID_CollosSelf,
	TexID_CollosValSelf,
	TexID_FlorenceSelf,
	TexID_Fountain,
	TexID_LeatherSchool,
	TexID_LeatherShop,

	//books
	TexID_CodingBooks,
	TexID_DarkSouls,
	TexID_History,
	TexID_HorrorBooks,
	TexID_LangBooks,
	TexID_NerdBooks,
	TexID_ShowBooks,

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
#define MAX_TEXTURE_SLOTS 32
struct RendererTextureBuffer {
	u32 max_texture_slots;
	TextureDim dimension;
	Texture textures[MAX_TEXTURE_SLOTS];
};

struct TextureIdSrcPair {
	TextureID id;
	char src[20];
};
