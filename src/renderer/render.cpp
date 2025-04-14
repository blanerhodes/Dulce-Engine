#include "render.h"
#include "texture_gen.h"
#include "dulce.h"
#include "wfobj_loader.h"
#include "renderer_shape_gen.h"
#include "defines.h"
#include "dmemory.h"
#include "game_input.h"
#include "dstring.h"
#include "dxmath.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../vendor/stb/stb_image.h"
//NOTE: allowing only png and bmp right now
//#define STBI_NO_JPEG
//#define STBI_NO_PSD
//#define STBI_NO_TGA
//#define STBI_NO_GIF
//#define STBI_NO_HDR
//#define STBI_NO_PIC
//#define STBI_NO_PNM

/*NOTE: shape types to still do
        - lines
		- spheres
*/

void D3DCreateTextureResource(Texture* texture, u32 dimension);

//TODO: Maybe redo this api to mimic more like the windows virtual alloc call
//      i.e. have a reserve call, commit call, write call, and make combos of them as usage comes up
RendererCommandBuffer* RendererInitCommandBuffer(MemoryArena* arena, u32 buffer_size) {
	RendererCommandBuffer* buffer = PushStruct(arena, RendererCommandBuffer);
	buffer->base_address = PushSize(arena, buffer_size);
	buffer->used_memory_size = 0;
	buffer->max_memory_size = buffer_size;
	buffer->command_count = 0;
	return buffer;
}

void RendererInitCommandBuffer(RendererState* state, u32 buffer_size) {
	state->command_buffer = RendererInitCommandBuffer(&state->permanent_storage, buffer_size);
}

void RendererResetCommandBuffer(RendererCommandBuffer* buffer) {
	buffer->used_memory_size = 0;
	buffer->command_count = 0;
}

//#define PushRenderCommand(buffer, type) (type*)PushRenderCommand_(buffer, sizeof(type), RenderType_##type)
void PushRenderCommand(RendererCommandBuffer* buffer, RenderCommand command) {
	if (buffer->used_memory_size + sizeof(command) < buffer->max_memory_size) {
		//TODO: look into a more succint way to do this in one line instead of two
		RenderCommand* command_memory_block = (RenderCommand*)(buffer->base_address + buffer->used_memory_size);
		*command_memory_block = command;
		buffer->used_memory_size += sizeof(command);
		buffer->command_count++;
	}
	else {
		INVALID_CODE_PATH;
	}
}

RendererVertexBuffer* RendererInitVertexBuffer(MemoryArena* arena, u32 buffer_size) {
	RendererVertexBuffer* buffer = PushStruct(arena, RendererVertexBuffer);
	buffer->base_address = PushSize(arena, buffer_size);
	buffer->used_memory_size = 0;
	buffer->max_memory_size = buffer_size;
	buffer->vertex_count = 0;
	return buffer;
}

void RendererInitVertexBuffer(RendererState* state, u32 buffer_size) {
	state->vertex_buffer = RendererInitVertexBuffer(&state->permanent_storage, buffer_size);
}

void RendererResetVertexBuffer(RendererVertexBuffer* buffer) {
	buffer->used_memory_size = 0;
	buffer->vertex_count = 0;
}

//TODO: figure out if there is a way to write directly to the memory returned here insted of doing a memcpy
//#define RendererCommitVertexMemory(buffer, count, type) (type*)RendererCommitVertexMemory_(buffer, count, sizeof(type))
void RendererCommitVertexMemory(RendererVertexBuffer* buffer, void* data, u32 count) {
	u32 size = sizeof(Vertex) * count;
	if (buffer->used_memory_size + size < buffer->max_memory_size) {
		//TODO: test if casting this equation to Vertex will mess up the addition
		MemCopy(data, buffer->base_address+buffer->used_memory_size, size);
		buffer->used_memory_size += size;
		buffer->vertex_count += count;
	}
	else {
		INVALID_CODE_PATH;
	}
}

RendererIndexBuffer* RendererInitIndexBuffer(MemoryArena* arena, u32 buffer_size) {
	RendererIndexBuffer* buffer = PushStruct(arena, RendererIndexBuffer);
	buffer->base_address = PushSize(arena, buffer_size);
	buffer->used_memory_size = 0;
	buffer->max_memory_size = buffer_size;
	buffer->index_count = 0;
	return buffer;
}

void RendererInitIndexBuffer(RendererState* state, u32 buffer_size) {
	state->index_buffer = RendererInitIndexBuffer(&state->permanent_storage, buffer_size);
}

void RendererResetIndexBuffer(RendererIndexBuffer* buffer) {
	buffer->used_memory_size = 0;
	buffer->index_count = 0;
}

void RendererCommitIndexMemory(RendererIndexBuffer* buffer, void* data, u32 count) {
	u32 size = sizeof(Index) * count;
	if (buffer->used_memory_size + size < buffer->max_memory_size) {
		MemCopy(data, buffer->base_address+buffer->used_memory_size, size);
		buffer->used_memory_size += size;
		buffer->index_count += count;
	}
	else {
		INVALID_CODE_PATH;
	}
}

RendererConstantBuffer* RendererInitConstantBuffer(MemoryArena* arena, u32 slot_count, u32 slot_size) {
	u32 buffer_size = slot_count * slot_size;
	RendererConstantBuffer* buffer = PushStruct(arena, RendererConstantBuffer);
	buffer->base_address = PushSize(arena, buffer_size);
	buffer->max_slots = slot_count;
	buffer->slot_size = slot_size;
	buffer->slots_used = 0;
	u32 slot_index = 0;
	for (u32 i = 0; i < buffer->max_slots; i++) {
		buffer->slot_addresses[i] = 0;
	}
	return buffer;
}

void RendererInitConstantBuffers(RendererState* state, u32 vs_obj_slot_count, 
									u32 vs_obj_slot_size, u32 vs_frame_slot_count, 
									u32 vs_frame_slot_size, u32 ps_frame_slot_count, 
									u32 ps_frame_slot_size) {

	state->vs_obj_constant_buffer = RendererInitConstantBuffer(&state->permanent_storage, vs_obj_slot_count, vs_obj_slot_size);
	state->vs_frame_constant_buffer = RendererInitConstantBuffer(&state->permanent_storage, vs_frame_slot_count, vs_frame_slot_size);
	state->ps_frame_constant_buffer = RendererInitConstantBuffer(&state->permanent_storage, ps_frame_slot_count, ps_frame_slot_size);
}

void RendererConstantBufferClear(RendererConstantBuffer* buffer) {
	buffer->slots_used = 0;
	for (u32 i = 0; i < buffer->max_slots; i++) {
		buffer->slot_addresses[i] = 0;
	}
}

u32 RendererConstantBufferGetNextFree(RendererConstantBuffer* buffer) {
	for (u32 i = 0; i < buffer->max_slots; i++) {
		if (buffer->slot_addresses[i] == 0) {
			return i;
		}
	}
	INVALID_CODE_PATH
	return buffer->max_slots; //NOTE: need to make dealing with a full constant buffer more robust
}

u32 RendererConstantBufferCommit(RendererConstantBuffer* buffer, void* data) {
	u32 slot = RendererConstantBufferGetNextFree(buffer);
	buffer->slot_addresses[slot] = buffer->base_address + slot*buffer->slot_size;
	MemCopy(data, buffer->slot_addresses[slot], buffer->slot_size);
	return slot * buffer->slot_size;
}

u32 RendererCommitConstantVSObjectMemory(RendererState* state, void* data) {
	return RendererConstantBufferCommit(state->vs_obj_constant_buffer, data);
}

u32 RendererCommitConstantVSFrameMemory(RendererState* state, void* data) {
	return RendererConstantBufferCommit(state->vs_frame_constant_buffer, data);
}

u32 RendererCommitConstantPSFrameMemory(RendererState* state, void* data) {
	return RendererConstantBufferCommit(state->ps_frame_constant_buffer, data);
}

//TODO: eventually load this info from a file instead of hardcoding it here
void RendererInitTextureIdTable(RendererState* renderer) {
	renderer->texture_ids[0] = { TexID_Unset, ""};
	renderer->texture_ids[1] = { TexID_Default, ""};
	renderer->texture_ids[2] = { TexID_WhiteTexture, "" };
	renderer->texture_ids[3] = { TexID_Sky, ""};
	renderer->texture_ids[4] = { TexID_Pic, ""};
}

RendererTextureBuffer* RendererInitTextureBuffer(MemoryArena* arena, TextureDim dimension){
	RendererTextureBuffer* buffer = PushStruct(arena, RendererTextureBuffer);

	buffer->textures[TexID_Default].id = TexID_Default;
	buffer->textures[TexID_Default].data = PushSize(arena, dimension*dimension*sizeof(u32));
	RendererGenDefaultTexture(buffer->textures[TexID_Default].data, 128);
	D3DCreateTextureResource(&buffer->textures[TexID_Default], dimension, dimension);

	buffer->textures[TexID_WhiteTexture].id = TexID_WhiteTexture;
	buffer->textures[TexID_WhiteTexture].data = PushSize(arena, dimension*dimension*sizeof(u32));
	RendererGenWhiteTexture(buffer->textures[TexID_WhiteTexture].data, 128);
	D3DCreateTextureResource(&buffer->textures[TexID_WhiteTexture], dimension, dimension);

	i32 width = 0;
	i32 height = 0;
	i32 bpp;
	u8* image = stbi_load("C:\\dev\\d3d_proj\\resources\\assets\\awesomeface.png", &width, &height, &bpp, 4);
	buffer->textures[TexID_Pic].id = TexID_Pic;
	buffer->textures[TexID_Pic].data = PushSize(arena, width*height*sizeof(u32));
	MemCopy(image, buffer->textures[TexID_Pic].data, width*height*bpp);
	D3DCreateTextureResource(&buffer->textures[TexID_Pic], width, height);

	for (u32 tex_index = 3; tex_index < ArrayCount(buffer->textures); tex_index++) {
		buffer->textures[tex_index].id = TexID_Unset;
		buffer->textures[tex_index].data = PushSize(arena, dimension*dimension*sizeof(u32));
	}
	buffer->max_texture_slots = ArrayCount(buffer->textures);
	return buffer;
}

void RendererInitTextureBuffer(RendererState* state, TextureDim dimension) {
	state->texture_buffer = RendererInitTextureBuffer(&state->permanent_storage, dimension);
}

void RendererResetTextureBuffer(RendererTextureBuffer* texture_buffer) {
	//go through and zero out all the memory for each texture??
}


//TODO: instead of a switch have a system of looking up in a table of texture ids and their source to retrieve them
//      if they arent present in the buffer
// return struct of texturebuffer/index when this uses 128/512... etc buffers
u32 RendererLoadTexture(RendererState* renderer, u32 texture_id) {
	RendererTextureBuffer* buffer = renderer->texture_buffer;
	//parse texture_id
	//see if it's in the buffer already and if not go load it into the buffer
	//this needs to be a hashmap probably at some point
	for (u32 tex_index = 0; tex_index < buffer->max_texture_slots; tex_index++) {
		if (buffer->textures[tex_index].id == texture_id) {
			return tex_index;
		}
	}

	u32 buffer_index_result = buffer->max_texture_slots;
	for (u32 tex_index = 0; tex_index < buffer->max_texture_slots; tex_index++) {
		if (buffer->textures[tex_index].id == TexID_Unset) {
			buffer_index_result = tex_index;
			break;
		}
	}
	DASSERT(buffer_index_result < buffer->max_texture_slots);
	
	switch (texture_id) {
		case TexID_Default: {
			buffer->textures[buffer_index_result].id = TexID_Default;
			DWARN("TexID_Default was requested from RendererLoadTexture()");
		} break;
		case TexID_WhiteTexture: {
			buffer->textures[buffer_index_result].id = TexID_WhiteTexture;
		} break;
		case TexID_Sky: {
			buffer->textures[buffer_index_result].id = TexID_Sky;
			RendererGenGradientTexture(buffer->textures[buffer_index_result].data, buffer->dimension, COLOR_BLUE);
			D3DCreateTextureResource(&buffer->textures[buffer_index_result], buffer->dimension);
		} break;
		case TexID_Pic: {
			buffer->textures[buffer_index_result].id = TexID_Pic;
		} break;
		default: {INVALID_CODE_PATH;}
	}
	
	return buffer_index_result;
}

DirectX::XMMATRIX ApplyViewProjection(DirectX::XMMATRIX model_transform, RendererState* renderer) {
	return DirectX::XMMatrixTranspose(model_transform * renderer->view * renderer->projection);
}

void RendererCommitToBuffers(RendererState* renderer, Vertex* vertices, u32 vert_count, Index* indices, u32 index_count, BasicMesh mesh, bool use_scale = true) {

	RendererCommitVertexMemory(renderer->vertex_buffer, vertices, vert_count);
	RendererCommitIndexMemory(renderer->index_buffer, indices, index_count);

	DirectX::XMMATRIX transform = DXGenTransform(mesh, use_scale);
	DirectX::XMMATRIX mvp = ApplyViewProjection(transform, renderer);
	PerObjectConstants constants = {DirectX::XMMatrixTranspose(transform), mvp};
	u32 const_buff_offset = RendererCommitConstantVSObjectMemory(renderer, &constants);

	if (mesh.texture_id != TexID_Unset) {
		RendererLoadTexture(renderer, mesh.texture_id);
	}

	RenderCommand command = {
		.vertex_count = vert_count,
		.vertex_buffer_offset = renderer->vertex_buffer->vertex_count - vert_count, 
		.index_count = index_count,
		.index_buffer_offset = renderer->index_buffer->index_count - index_count,
		.vertex_constant_buffer_offset = const_buff_offset,
		.topology = RenderTopology_TriangleList,
		.texture_id = mesh.texture_id,
	};
	PushRenderCommand(renderer->command_buffer, command);
}

void RendererPushPlane(RendererState* renderer, BasicMesh mesh) {
	Vertex vertices[] = {
		{ {-0.5f,  0.5f, 0.0f}, COLOR_REDA, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
		{ {0.5f,  0.5f, 0.0f}, COLOR_BLUEA, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
		{ {0.5f, -0.5f, 0.0f}, COLOR_GREENA, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
		{ {-0.5f, -0.5f, 0.0f}, COLOR_BLACKA, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}
	};
	Index indices[] = {
		0, 1, 3,
		1, 2, 3
	};
	RendererCommitToBuffers(renderer, vertices, ArrayCount(vertices), indices, ArrayCount(indices), mesh);
}

void RendererPushCube(RendererState* renderer, BasicMesh mesh) {
	f32 side = 0.5f;
	Vertex vertices[] = {
		{{-side, -side, -side}, COLOR_REDA,     {0.0f, 0.0f}, { 0.0f,  0.0f, -1.0f}}, //0 near side
		{{ side, -side, -side}, COLOR_REDA,     {0.0f, 0.0f}, { 0.0f,  0.0f, -1.0f}}, //1
		{{-side,  side, -side}, COLOR_REDA,     {0.0f, 0.0f}, { 0.0f,  0.0f, -1.0f}}, //2
		{{ side,  side, -side}, COLOR_REDA,     {0.0f, 0.0f}, { 0.0f,  0.0f, -1.0f}}, //3
		{{-side, -side,  side}, COLOR_GREENA,   {0.0f, 0.0f}, { 0.0f,  0.0f,  1.0f}}, //4 far side
		{{ side, -side,  side}, COLOR_GREENA,   {0.0f, 0.0f}, { 0.0f,  0.0f,  1.0f}}, //5
		{{-side,  side,  side}, COLOR_GREENA,   {0.0f, 0.0f}, { 0.0f,  0.0f,  1.0f}}, //6
		{{ side,  side,  side}, COLOR_GREENA,   {0.0f, 0.0f}, { 0.0f,  0.0f,  1.0f}},  //7
		{{-side, -side, -side}, COLOR_BLUEA,    {0.0f, 0.0f}, {-1.0f,  0.0f,  0.0f}}, //8 left side
		{{-side,  side, -side}, COLOR_BLUEA,    {0.0f, 0.0f}, {-1.0f,  0.0f,  0.0f}}, //9
		{{-side, -side,  side}, COLOR_BLUEA,    {0.0f, 0.0f}, {-1.0f,  0.0f,  0.0f}}, //10
		{{-side,  side,  side}, COLOR_BLUEA,    {0.0f, 0.0f}, {-1.0f,  0.0f,  0.0f}}, //11
		{{ side, -side, -side}, COLOR_CYANA,    {0.0f, 0.0f}, { 1.0f,  0.0f,  0.0f}}, //12 right side
		{{ side,  side, -side}, COLOR_CYANA,    {0.0f, 0.0f}, { 1.0f,  0.0f,  0.0f}}, //13
		{{ side, -side,  side}, COLOR_CYANA,    {0.0f, 0.0f}, { 1.0f,  0.0f,  0.0f}}, //14
		{{ side,  side,  side}, COLOR_CYANA,    {0.0f, 0.0f}, { 1.0f,  0.0f,  0.0f}}, //15
		{{-side, -side, -side}, COLOR_MAGENTAA, {0.0f, 0.0f}, { 0.0f, -1.0f,  0.0f}}, //16 bottom side
		{{ side, -side, -side}, COLOR_MAGENTAA, {0.0f, 0.0f}, { 0.0f, -1.0f,  0.0f}}, //17
		{{-side, -side,  side}, COLOR_MAGENTAA, {0.0f, 0.0f}, { 0.0f, -1.0f,  0.0f}}, //18
		{{ side, -side,  side}, COLOR_MAGENTAA, {0.0f, 0.0f}, { 0.0f, -1.0f,  0.0f}}, //19
		{{-side,  side, -side}, COLOR_YELLOWA,  {0.0f, 0.0f}, { 0.0f,  1.0f,  0.0f}}, //20 top side
		{{ side,  side, -side}, COLOR_YELLOWA,  {0.0f, 0.0f}, { 0.0f,  1.0f,  0.0f}}, //21
		{{-side,  side,  side}, COLOR_YELLOWA,  {0.0f, 0.0f}, { 0.0f,  1.0f,  0.0f}}, //22
		{{ side,  side,  side}, COLOR_YELLOWA,  {0.0f, 0.0f}, { 0.0f,  1.0f,  0.0f}} //23
	};

	Index indices[] = {
		0,2,1, 2,3,1,
		4,5,7, 4,7,6,
		8,10,9, 10,11,9,
		12,13,15, 12,15,14,
		16,17,18, 18,17,19,
		20,23,21, 20,22,23 
	};

	RendererCommitToBuffers(renderer, vertices, ArrayCount(vertices), indices, ArrayCount(indices),  mesh);
}

void RendererPushCone(RendererState* renderer, BasicMesh mesh) {
	u32 num_verts = mesh.scale.divisions + 1 + mesh.scale.divisions * 3;
	u32 num_indices = mesh.scale.divisions * 6;

	RendererVertexBuffer* vertex_buffer = renderer->vertex_buffer;
	RendererIndexBuffer* index_buffer = renderer->index_buffer;
	//RendererConstantBuffer* constant_buffer = renderer->vertex_constant_buffer;

	Vertex* vertices = PushArray(&renderer->scratch_storage, num_verts, Vertex);
	Index* indices = PushArray(&renderer->scratch_storage, num_indices, Index);
	Vec3 root_pos = {};
	u32 num_verts_made_base = RendererGenCircularBaseVertices(renderer, &mesh, root_pos, vertices);
	Vec3 peak_pos = {root_pos.x, root_pos.y+mesh.scale.height, root_pos.z};
	u32 num_verts_made_uprights = RendererGenConicalUprightVertices(renderer, &mesh, peak_pos, vertices+num_verts_made_base);

	u32 num_base_indices_made = RendererGenCircularBaseIndices(renderer, &mesh, 0, 1, indices, false);
	RendererGenConicalUprightIndices(renderer, &mesh, num_verts_made_base, indices+num_base_indices_made);

	RendererCommitToBuffers(renderer, vertices, num_verts, indices, num_indices, mesh, false);
}

void RendererPushPyramid(RendererState* renderer, BasicMesh mesh) {
	RendererPushCone(renderer, mesh);
}

void RendererPushCylinder(RendererState* renderer, BasicMesh mesh) {
	u32 num_verts = mesh.scale.divisions * 2 + 2 + mesh.scale.divisions * 4;
	u32 num_indices = mesh.scale.divisions * 12; //NOTE: div*3 top, div*3 bot, div*6 sides

	Vertex* vertices = PushArray(&renderer->scratch_storage, num_verts, Vertex);
	Index* indices = PushArray(&renderer->scratch_storage, num_indices, Index);
	Vec3 root_pos = {};
	u32 num_base_verts_made = RendererGenCircularBaseVertices(renderer, &mesh, root_pos, vertices);
	Vec3 top_center_pos = {root_pos.x, root_pos.y + mesh.scale.height, root_pos.z};
	u32 num_top_verts_made = RendererGenCircularBaseVertices(renderer, &mesh, top_center_pos, vertices+num_base_verts_made, true);
	u32 num_upright_verts_made = RendererGenCylindricalUprightVertices(renderer, &mesh, root_pos, vertices+num_base_verts_made+num_top_verts_made);
	u32 num_base_indices_made = RendererGenCircularBaseIndices(renderer, &mesh, 0, 1, indices, false);
	u32 num_top_indices_made = RendererGenCircularBaseIndices(renderer, &mesh, num_base_verts_made,
		                                                      num_base_verts_made+1, indices+num_base_indices_made, true);
	u32 num_wrapped_indices_made = RendererGenCircularWrapIndices(renderer, &mesh, num_base_verts_made+num_top_verts_made,
		                                                          indices+num_base_indices_made+num_top_indices_made);
	RendererCommitToBuffers(renderer, vertices, num_verts, indices, num_indices, mesh, false);
}

void RendererPushAsset(RendererState* renderer, BasicMesh mesh) {
	AssetData* asset = AssetHashMapGet(&renderer->loaded_assets, mesh.asset_id);
	if (!asset) {
		AssetData new_asset = {.id = mesh.asset_id};
		LoadDOF(&renderer->permanent_storage, &new_asset, RendererLookupAsset(renderer, mesh.asset_id));
		asset = AssetHashMapInsert(&renderer->loaded_assets, new_asset);
	}

	RendererCommitToBuffers(renderer, asset->vertices, asset->vertex_count, asset->indices, asset->index_count, mesh);
}

void RendererPushGrid(RendererState* renderer, f32 width, f32 depth, u32 rows, u32 cols, BasicMesh mesh) {

	u32 vertex_count = rows * cols;
	u32 face_count = (rows-1) * (cols-1) * 2;
	u32 index_count = face_count * 3;

	f32 half_width = 0.5f * width;
	f32 half_depth = 0.5f * depth;
	f32 dx = width / (cols-1);
	f32 dy = depth / (rows-1);
	f32 du = 1.0f / (cols-1);
	f32 dv = 1.0f / (rows-1);

	Vertex* vertices = PushArray(&renderer->scratch_storage, vertex_count, Vertex);
	for (u32 i = 0; i < rows; i++) {
		f32 y = half_depth - i*dy;
		for (u32 j = 0; j < cols; j++) {
			f32 x = -half_width + j*dx;
			vertices[i*cols+j].position = {x, y, 0.0f};
			vertices[i*cols+j].normal = {0.0f, 0.0f, 1.0f};
			vertices[i*cols+j].color = mesh.color;
			//vertices[i*rows+j].tangent_u = {1.0f, 0.0f, 0.0f};
			vertices[i*cols+j].tex_coord.x = j*du;
			vertices[i*cols+j].tex_coord.y = i*dv;
		}
	}

	Index* indices = PushArray(&renderer->scratch_storage, index_count, Index);
	u32 k = 0;
	for (u32 i = 0; i < rows-1; i++) {
		for (u32 j = 0; j < cols-1; j++) {
			indices[k].value   = i*cols + j;
			indices[k+1].value = i*cols + j+1;
			indices[k+2].value = (i+1)*cols + j;
			indices[k+3].value = (i+1)*cols + j;
			indices[k+4].value = i*cols + j+1;
			indices[k+5].value = (i+1)*cols + j+1;
			k += 6;
		}
	}

	RendererCommitToBuffers(renderer, vertices, vertex_count, indices, index_count, mesh);
}

u8* RendererLookupAsset(RendererState* renderer, AssetID id) {
	for (u32 i = 0; i < renderer->asset_count; i++) {
		if (StringsEqual(renderer->assets_table[i].name, id.name)) {
			return renderer->assets_table[i].filepath;
		}
	}
	return NULL;
}


//NOTE: this is hardcoding a point light to be in slot 0 of the constant buffer
void RendererPushPointLight(RendererState* renderer) {
	RendererCommitConstantPSFrameMemory(renderer, &renderer->ps_pfc);
}

void RendererPushClear(Vec3 color) {
	Vec4 color4 = {color.r, color.g, color.b, 1.0f};
    g_d3d.context->ClearRenderTargetView(g_d3d.frame_buffer_view, (f32*)&color4);
    g_d3d.context->ClearDepthStencilView(g_d3d.depth_buffer_view, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void RendererPerFrameReset(GameState* game_state, RendererState* renderer_state, GameInput* input) {
    //TODO: at some point im guessing reseting like this will be a bad idea but wfk
    RendererResetVertexBuffer(renderer_state->vertex_buffer);
    RendererResetIndexBuffer(renderer_state->index_buffer);
    RendererResetCommandBuffer(renderer_state->command_buffer);
    //TODO: see if it matters/worth it to only reset everything other than the projection matrix
    RendererConstantBufferClear(renderer_state->vs_frame_constant_buffer);
    RendererConstantBufferClear(renderer_state->vs_obj_constant_buffer);
    RendererConstantBufferClear(renderer_state->ps_frame_constant_buffer);
    ArenaReset(&renderer_state->scratch_storage);

    CameraUpdate(&game_state->camera, input);
	renderer_state->view = CameraGetViewMatrix(&game_state->camera);	

    RendererPushClear({ 0.5f, 0.5f, 0.5f });
}

u32 BinarySearch(u32 arr[], u32 size, u32 target) {
	u32 left = 0;
	u32 right = size-1;
	u32 boundary = 0;
	while (left <= right) {
		u32 mid = left + (right - left) / 2;
		if (arr[mid] >= target) {
			boundary = mid;
			right = mid-1;
		}
		else {
			left = mid + 1;
		}
	}
	return arr[boundary];
}

u32 primes[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 
                 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97,
	             101, 103, 107, 109, 113, 127, 131 };
void InitAssetHashMap(MemoryArena* arena, AssetHashMap* hashmap, u32 num_slots) {
	u32 first_prime = BinarySearch(primes, 25, num_slots);
	hashmap->slots = PushArray(arena, first_prime, AssetData);
	hashmap->num_slots = first_prime;
}

u32 AssetIDHash(AssetID id, u32 map_size) {
	u32 hash = 0;
	u8* name = id.name;
	for (u32 i = 0; *name; i++, name++) {
		hash += *name * primes[i];
	}
	return hash % map_size;
}

AssetData* AssetHashMapGet(AssetHashMap* hashmap, AssetID id){
	u32 hash = AssetIDHash(id, hashmap->num_slots);
	if (!hashmap->slots[hash].vertices) {
		return 0;
	}
	for (u32 index = 0; index < hashmap->num_slots; index++) {
		u32 ind_hash = (index + hash) % hashmap->num_slots;
		if (!hashmap->slots[ind_hash].vertices) {
			break;
		}
		if (StringsEqual(hashmap->slots[ind_hash].id.name, id.name)) {
			return &hashmap->slots[ind_hash];
		}
	}
	return 0;
}

AssetData* AssetHashMapInsert(AssetHashMap* hashmap, AssetData asset) {
	u32 hash = AssetIDHash(asset.id, hashmap->num_slots);
	for (u32 index = 0; index < hashmap->num_slots; index++) {
		u32 ind_hash = (index + hash) % hashmap->num_slots;
		if (!hashmap->slots[ind_hash].vertices) {
			AssetData* empty_slot = &hashmap->slots[ind_hash];
			MemCopy(&asset, empty_slot, sizeof(AssetData));
			return empty_slot;
		}
	}
	return 0;
}

void LoadAssetConfig(RendererState* renderer) {
    char* filename = "../../resources/assets/assets.ini";
    DebugReadFileResult read_result = DebugPlatformReadEntireFile(0, filename);
    renderer->assets_table = (AssetLookup*)PushSize(&renderer->permanent_storage, 0);
    renderer->asset_count = 0;
    u8* file_ptr = (u8*)read_result.contents;
    u32 num_copied = 0;
//TODO: figure out why the string copy isnt working
    while (num_copied < read_result.contents_size) {
		AssetLookup curr = {};
		num_copied += StringCopyToWS(file_ptr+num_copied, curr.name, true);
   		num_copied += StringCopyToWS(file_ptr+num_copied, curr.filepath, true);
   		AssetLookup* next_id = PushStruct(&renderer->permanent_storage, AssetLookup);
   		MemCopy(&curr, next_id, sizeof(AssetLookup));
   		renderer->asset_count++;
    }
}

void RendererInitAssets(RendererState* state, u32 num_slots) {
	LoadAssetConfig(state);
	InitAssetHashMap(&state->permanent_storage, &state->loaded_assets, num_slots);
}

void RendererPushProfilingUI(RendererState* renderer, ThreadContext* thread) {
//0 const buffer offset
}
