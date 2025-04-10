#pragma once
#include "texture.h"
#include "defines.h"
#include "dmath.h"
#include "dmemory.h"
#include "game_input.h"
#include "dulce.h"
#include <DirectXMath.h>

#define COLOR_RED	   {1.0f, 0.0f, 0.0f}
#define COLOR_GREEN	   {0.0f, 1.0f, 0.0f}
#define COLOR_BLUE	   {0.0f, 0.0f, 1.0f}
#define COLOR_WHITE	   {1.0f, 1.0f, 1.0f}
#define COLOR_BLACK	   {0.0f, 0.0f, 0.0f}
#define COLOR_MAGENTA   {1.0f, 0.0f, 1.0f}
#define COLOR_YELLOW   {1.0f, 1.0f, 0.0f}
#define COLOR_GREY	   {0.5f, 0.5f, 0.5f}
#define COLOR_CYAN	   {0.0f, 1.0f, 1.0f}
#define COLOR_NO_COLOR {1.0f, 1.0f, 1.0f}

#define COLOR_REDA	    {1.0f, 0.0f, 0.0f, 1.0f}
#define COLOR_GREENA    {0.0f, 1.0f, 0.0f, 1.0f}
#define COLOR_BLUEA	    {0.0f, 0.0f, 1.0f, 1.0f}
#define COLOR_WHITEA	{1.0f, 1.0f, 1.0f, 1.0f}
#define COLOR_BLACKA	{0.0f, 0.0f, 0.0f, 1.0f}
#define COLOR_MAGENTAA   {1.0f, 0.0f, 1.0f, 1.0f}
#define COLOR_YELLOWA   {1.0f, 1.0f, 0.0f, 1.0f}
#define COLOR_GREYA	    {0.5f, 0.5f, 0.5f, 1.0f}
#define COLOR_CYANA	    {0.0f, 1.0f, 1.0f, 1.0f}
#define COLOR_NO_COLORA {1.0f, 1.0f, 1.0f, 1.0f}

enum DefaultColorsU32 {
	DefaultColors_Red     = (255<<24) | (255<<16) | (0<<8)   | 0,
	DefaultColors_Green   = (255<<24) | (0<<16)   | (255<<8) | 0,
	DefaultColors_Blue    = (255<<24) | (0<<16)   | (0<<8)   | 255,
	DefaultColors_White   = (255<<24) | (255<<16) | (255<<8) | 255,
	DefaultColors_Black   = (255<<24) | (0<<16)   | (0<<8)   | 0,
	DefaultColors_Magenta = (255<<24) | (255<<16) | (0<<8)   | 255,
	DefaultColors_Yellow  = (255<<24) | (255<<16) | (255<<8) | 0,
	DefaultColors_Grey    = (255<<24) | (128<<16) | (128<<8) | 128,
	DefaultColors_Cyan    = (255<<24) | (0<<16)   | (128<<8) | 128,
	DefaultColors_Count   = 9
};

enum RenderTopology {
	RenderTopology_TriangleList,
	RenderTopology_LineList,
	RenderTopology_PointList
};

enum PixelShaderType {
	PixelShaderType_Textured,
	PixelShaderType_Untextured,
};

#pragma pack(push, 4)
struct Vertex {   
    Vec3 position;
	Vec4 color;
	Vec2 tex_coord;
	Vec3 normal;
};
#pragma pack(pop)

bool operator==(const Vertex& lhs, const Vertex& rhs) {
	bool result = (lhs.position == rhs.position) && 
		          (lhs.normal == rhs.normal) && 
		          (lhs.color == rhs.color) && 
		          (lhs.tex_coord == lhs.tex_coord);
	return result;
}

struct Index {
	u16 value;
};

struct AssetID {
	u8 name[32];
};

struct AssetLookup {
	u8 name[32];
	u8 filepath[64];
};

struct AssetData {
	AssetID id;
	u32 vertex_count;
	Vertex* vertices;
	u32 index_count;
	Index* indices;
	u32 ref_count;
};

struct AssetHashMap {
	AssetData* slots;
	u32 num_slots;
};

//TODO: not sure if i need to pass the transform anymore since i have the constant buffer setup now
struct RenderCommand {
	u32 vertex_count;
	u32 vertex_buffer_offset;
	u32 index_count;
	u32 index_buffer_offset;
	u32 vertex_constant_buffer_offset;
	RenderTopology topology;
	i32 texture_id;
};

struct RendererCommandBuffer {
	u8* base_address;
	u32 command_count;
	u32 used_memory_size;
	u32 max_memory_size;
};

struct RendererVertexBuffer {
	u8* base_address;
	u32 vertex_count;
	u32 used_memory_size;
	u32 max_memory_size;
};

struct RendererIndexBuffer {
	u8* base_address;
	u32 index_count;
	u32 used_memory_size;
	u32 max_memory_size;
};

struct RendererScratchBuffer {
	u8* base_address;
	u32 used_memory_size;
	u32 max_memory_size;
};


//TODO: find out later if i need to have an array of pointers to the different slots and maintain a free list
//NOTE: not sure if this should be backed by its own pool allocator
//NOTE: POOL ALLOC DOESNT WORK because i need random access, an idea maybe something like a SlottedBuffer??
//      if i find another use case for this kind of thing i may break some of this out to a SlottedBuffer (basically a pool with random access)
#define MAX_UNIFORM_BUFFER_SLOTS 16
#define UNIFORM_BUFFER_SLOT_SIZE 256
struct RendererConstantBuffer {
	u8* base_address;
	u32 max_slots;
	u32 slot_size;
	u32 per_frame_slots_used;
	u32 per_object_slots_used;
	u32 num_per_frame_slots;
	u32 num_per_object_slots;
	u8* slot_addresses[MAX_UNIFORM_BUFFER_SLOTS];
	u32 slot_space_used[MAX_UNIFORM_BUFFER_SLOTS];
};

struct RendererMemory {
	b32 is_initialized;
	u64 permanent_storage_size;
	void* permanent_storage;
	u64 scratch_storage_size;
	void* scratch_storage;
};

struct PointLight {
	alignas(16) Vec3 position;
	alignas(16) Vec3 mat_color;
	alignas(16) Vec3 ambient;
	alignas(16) Vec3 diffuse_color;
	f32 diffuse_intensity;
	f32 att_const;
	f32 att_lin;
	f32 att_quad;
};

struct PerObjectConstants {
	DirectX::XMMATRIX model_transform;
	DirectX::XMMATRIX mvp;
};

struct PerFrameConstants {
	PointLight point_light;
};

#include <DirectXMath.h>

struct RendererState {
	MemoryArena permanent_storage;
	RendererCommandBuffer* command_buffer;
	RendererVertexBuffer* vertex_buffer;
	RendererIndexBuffer* index_buffer;
	RendererConstantBuffer* vertex_constant_buffer;
	RendererTextureBuffer* texture_buffer;

	MemoryArena scratch_storage;
	TextureIdSrcPair texture_ids[32];

	u32 asset_count;
	//TODO: this probably needs to be a hashmap too
	AssetLookup* assets_table;
	AssetHashMap loaded_assets;

	PerFrameConstants per_frame_constants;

	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};

struct BasicMesh {
	Vec3 position;
	Vec3 scale;
	Vec3 rotation_angles;
	Vec4 color;
	i32 texture_id;
	AssetID asset_id;
};

//struct DirectionalLight {
//	Vec3 position;
//	Vec3 direction;
//	Vec3 rotation_angles;
//	Vec3 color;
//	f32 intensity;
//};


struct DirectionalLight {
	Vec4 ambient;
	Vec4 diffuse;
	Vec4 specular;
	Vec3 direction;
	f32 pad;
};

struct SpotLight {
	Vec4 ambient;
	Vec4 diffuse;
	Vec4 specular;
	Vec3 position;
	f32 range;
	Vec3 direction;
	f32 spot;
	Vec3 attenuation;
	f32 pad;
};

struct TesselatedMesh {
	BasicMesh mesh;
	u32 divisions;
};

struct Material {
	Vec4 ambient;
	Vec4 diffuse;
	Vec4 specular; //specular.w = specular_power
	Vec4 reflect;
};

RendererCommandBuffer* RendererInitCommandBuffer(MemoryArena* arena, u32 buffer_size);
void RendererInitCommandBuffer(RendererState* state, u32 buffer_size);
void RendererResetCommandBuffer(RendererCommandBuffer* buffer);
void PushRenderCommand(RendererCommandBuffer* buffer, RenderCommand command);

RendererVertexBuffer* RendererInitVertexBuffer(MemoryArena* arena, u32 buffer_size);
void RendererInitVertexBuffer(RendererState* state, u32 buffer_size);
void RendererResetVertexBuffer(RendererVertexBuffer* buffer);
void RendererCommitVertexMemory(RendererVertexBuffer* buffer, void* data, u32 count);

RendererIndexBuffer* RendererInitIndexBuffer(MemoryArena* arena, u32 buffer_size);
void RendererInitIndexBuffer(RendererState* state, u32 buffer_size);
void RendererResetIndexBuffer(RendererIndexBuffer* buffer);
void RendererCommitIndexMemory(RendererIndexBuffer* buffer, void* data, u32 count);

RendererConstantBuffer* RendererInitConstantBuffer(MemoryArena* arena, u32 per_frame_slots = 1);
void RendererInitConstantBuffer(RendererState* state, u32 per_frame_slots = 1);
void RendererConstantBufferClear(RendererConstantBuffer* buffer);
u32 RendererConstantBufferGetNextFree(RendererConstantBuffer* buffer, u32 start_index = 0, u32 end_index = 0);
u32 RendererConstantBufferCommit(RendererConstantBuffer* buffer, void* data, u32 slot, u32 size = 0);
u32 RendererCommitConstantFrameMemory(RendererConstantBuffer* buffer, void* data, u32 size = 0);
u32 RendererCommitConstantObjectMemory(RendererConstantBuffer* buffer, void* data, u32 size = 0);

void RendererInitTextureIdTable(RendererState* renderer);
RendererTextureBuffer* RendererInitTextureBuffer(MemoryArena* arena, TextureDim dimension);
void RendererInitTextureBuffer(RendererState* state, TextureDim dimension);
void RendererResetTextureBuffer(RendererTextureBuffer* texture_buffer);
u32 RendererLoadTexture(RendererState* renderer, u32 texture_id);

void RendererPushPlane(RendererState* renderer, BasicMesh plane_data);
void RendererPushCubeIndFaces(RendererState* renderer, BasicMesh cube_data);
void RendererPushCube(RendererState* renderer, BasicMesh cube_data);
void RendererPushCone(RendererState* renderer, BasicMesh cone);

void RendererPushPyramid(RendererState* renderer, BasicMesh mesh);
void RendererPushCylinder(RendererState* renderer, BasicMesh cyl);
void RendererPushAsset(RendererState* renderer, BasicMesh mesh);
void RendererPushGrid(RendererState* renderer, f32 width, f32 depth, u32 rows, u32 cols, BasicMesh grid);

void RendererPushPointLight(RendererState* renderer);
void RendererPushClear(Vec3 color);

void RendererPerFrameReset(GameState* game_state, RendererState* renderer_state, GameInput* input);
u32 BinarySearch(u32 arr[], u32 size, u32 target);

u8* RendererLookupAsset(RendererState* renderer, AssetID id);

void InitAssetHashMap(MemoryArena* arena, AssetHashMap* hashmap, u32 num_slots);
u32 AssetIDHash(AssetID id, u32 map_size);
AssetData* AssetHashMapGet(AssetHashMap* hashmap, AssetID id);
AssetData* AssetHashMapInsert(AssetHashMap* hashmap, AssetData asset);
void LoadAssetConfig(RendererState* renderer);
void RendererInitAssets(RendererState* state, u32 num_slots);