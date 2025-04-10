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

//NOTE: this is forcing per frame data do be in front section and per object data to follow after that
RendererConstantBuffer* RendererInitConstantBuffer(MemoryArena* arena, u32 per_frame_slots) {
	u32 buffer_size = MAX_UNIFORM_BUFFER_SLOTS * UNIFORM_BUFFER_SLOT_SIZE;
	RendererConstantBuffer* buffer = PushStruct(arena, RendererConstantBuffer);
	buffer->base_address = PushSize(arena, buffer_size);
	buffer->max_slots = MAX_UNIFORM_BUFFER_SLOTS;
	buffer->slot_size = UNIFORM_BUFFER_SLOT_SIZE;
	buffer->num_per_frame_slots = per_frame_slots;
	buffer->num_per_object_slots = buffer->max_slots - per_frame_slots;
	buffer->per_frame_slots_used = 0;
	buffer->per_object_slots_used = 0;
	u32 slot_index = 0;
	for (u32 i = 0; i < buffer->max_slots; i++) {
		buffer->slot_addresses[i] = 0;
		buffer->slot_space_used[i] = 0;
	}
	//buffer->slot_addresses[0] = buffer->base_address;
	return buffer;
}

void RendererInitConstantBuffer(RendererState* state, u32 per_frame_slots) {
	state->vertex_constant_buffer = RendererInitConstantBuffer(&state->permanent_storage, per_frame_slots);
}

void RendererConstantBufferClear(RendererConstantBuffer* buffer) {
	buffer->per_frame_slots_used = 0;
	buffer->per_object_slots_used = 0;
	for (u32 i = 0; i < buffer->max_slots; i++) {
		buffer->slot_addresses[i] = 0;
		buffer->slot_space_used[i] = 0;
	}
	buffer->slot_addresses[0] = buffer->base_address;
}

u32 RendererConstantBufferGetNextFree(RendererConstantBuffer* buffer, u32 start_index, u32 end_index) {
	end_index = (end_index == 0) ? buffer->max_slots : end_index;
	for (u32 i = start_index; i < end_index; i++) {
		if (buffer->slot_addresses[i] == 0) {
			return i;
		}
	}
	return buffer->max_slots; //NOTE: need to make dealing with a full constant buffer more robust
}

u32 RendererConstantBufferCommit(RendererConstantBuffer* buffer, void* data, u32 slot, u32 size) {
	DASSERT(slot < buffer->max_slots);
	if (buffer->slot_addresses[slot] == 0) {
		DASSERT(size <= buffer->slot_size);
		buffer->slot_addresses[slot] = buffer->base_address + slot*buffer->slot_size;
		MemCopy(data, buffer->slot_addresses[slot], size);
		buffer->slot_space_used[slot] += size;
		if (slot > buffer->num_per_frame_slots) {
			buffer->per_object_slots_used++;
		} 
	} else {
		if (size == 0) {
			MemCopy(data, buffer->slot_addresses[slot], buffer->slot_size);
			buffer->slot_space_used[slot] = buffer->slot_size;
		} else {
			DASSERT(size <= buffer->slot_size - buffer->slot_space_used[slot]);
			u8* write_pos = buffer->slot_addresses[slot] + buffer->slot_space_used[slot];
			MemCopy(data, write_pos, size);
			buffer->slot_space_used[slot] += size;
		}
	}

	return slot * buffer->slot_size;
}

u32 RendererCommitConstantFrameMemory(RendererConstantBuffer* buffer, void* data, u32 size) {
	//u32 result_slot = RendererConstantBufferGetNextFree(buffer, 0, buffer->num_per_frame_slots);
	u32 result_slot = 0;
	DASSERT(result_slot < buffer->max_slots);
	u32 offset_from_base = RendererConstantBufferCommit(buffer, data, result_slot, size);
	return offset_from_base;
}

//NOTE: this returns a byte offset ---from buffer->per_object_data, NOT base_address---
u32 RendererCommitConstantObjectMemory(RendererConstantBuffer* buffer, void* data, u32 size) {
	u32 result_slot = RendererConstantBufferGetNextFree(buffer, buffer->num_per_frame_slots);
	DASSERT(result_slot < buffer->max_slots);
	u32 offset_from_base_addr = RendererConstantBufferCommit(buffer, data, result_slot, size);
	return offset_from_base_addr - buffer->slot_size * buffer->num_per_frame_slots;
}

//TODO: eventually load this info from a file instead of hardcoding it here
void RendererInitTextureIdTable(RendererState* renderer) {
	renderer->texture_ids[0] = { TexID_Unset, ""};
	renderer->texture_ids[1] = { TexID_Default, ""};
	renderer->texture_ids[2] = { TexID_WhiteTexture, "" };
	renderer->texture_ids[3] = { TexID_Sky, ""};
	renderer->texture_ids[4] = { TexID_Pic, ""};
}

//TODO: make the renderer specific texture resource creation agnostic in this
RendererTextureBuffer* RendererInitTextureBuffer(MemoryArena* arena, TextureDim dimension){
	RendererTextureBuffer* buffer = PushStruct(arena, RendererTextureBuffer);
	//buffer->dimension = dimension;
	//buffer->textures[0].id = TexID_Default;
	//buffer->textures[0].data = PushSize(arena, dimension*dimension*sizeof(u32));
	//RendererGenDefaultTexture(buffer->textures[0].data, dimension);
	//D3DCreateTextureResource(&buffer->textures[0], dimension);

	//buffer->textures[1].id = TexID_WhiteTexture;
	//buffer->textures[1].data = PushSize(arena, dimension*dimension*sizeof(u32));
	//RendererGenWhiteTexture(buffer->textures[1].data, dimension);
	//D3DCreateTextureResource(&buffer->textures[1], dimension);

	//stbi_set_flip_vertically_on_load(1);
	i32 width = 0;
	i32 height = 0;
	i32 bpp;

	u8* image = stbi_load("C:\\dev\\d3d_proj\\resources\\awesomeface.png", &width, &height, &bpp, 4);
	buffer->textures[2].id = TexID_Pic;
	buffer->textures[2].data = PushSize(arena, width*height*sizeof(u32));
	MemCopy(image, buffer->textures[2].data, width*height*bpp);
	D3DCreateTextureResource(&buffer->textures[2], width, height);

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
		default: {INVALID_CODE_PATH;}
	}
	
	return buffer_index_result;
}

DirectX::XMMATRIX ApplyViewProjection(DirectX::XMMATRIX model_transform, RendererState* renderer) {
	return DirectX::XMMatrixTranspose(model_transform * renderer->view * renderer->projection);
}

void RendererPushPlane(RendererState* renderer, BasicMesh mesh) {
	Vertex vertices_plane[] = {
		{ {-0.5f,  0.5f, 0.0f}, COLOR_REDA, {0.0f, 0.0f}},
		{ {0.5f,  0.5f, 0.0f}, COLOR_BLUEA, {1.0f, 0.0f}},
		{ {0.5f, -0.5f, 0.0f}, COLOR_GREENA, {1.0f, 1.0f}},
		{ {-0.5f, -0.5f, 0.0f}, COLOR_BLACKA, {0.0f, 1.0f}}
	};
	Index indices_plane[] = {
		0, 1, 3,
		1, 2, 3
	};

	RendererVertexBuffer* vertex_buffer = renderer->vertex_buffer;
	RendererIndexBuffer* index_buffer = renderer->index_buffer;
	RendererConstantBuffer* constant_buffer = renderer->vertex_constant_buffer;

	u32 vert_count = ArrayCount(vertices_plane);
	u32 index_count = ArrayCount(indices_plane);
	RendererCommitVertexMemory(vertex_buffer, vertices_plane, vert_count);
	RendererCommitIndexMemory(index_buffer, indices_plane, index_count);

	//DirectX::XMMATRIX transform = DXGenTransform(mesh, renderer->projection);
	DirectX::XMMATRIX transform = DXGenTransform(mesh, renderer);
	//transform = renderer->projection * transform;
	//transform = ApplyViewProjection(transform, renderer);
	PerObjectConstants constants = {transform};
	u32 const_buff_offset = RendererCommitConstantObjectMemory(constant_buffer, &constants, sizeof(PerObjectConstants));

	if (mesh.texture_id != TexID_NoTexture) {
		RendererLoadTexture(renderer, mesh.texture_id);
	}

	RenderCommand command = {
		.vertex_count = vert_count,
		.vertex_buffer_offset = vertex_buffer->vertex_count - vert_count, 
		.index_count = index_count,
		.index_buffer_offset = index_buffer->index_count - index_count,
		.vertex_constant_buffer_offset = const_buff_offset,
		.topology = RenderTopology_TriangleList,
		.texture_id = mesh.texture_id
	};
	PushRenderCommand(renderer->command_buffer, command);
}

void RendererPushCubeIndFaces(RendererState* renderer, BasicMesh cube_data) {
	RendererVertexBuffer* vertex_buffer = renderer->vertex_buffer;
	RendererIndexBuffer* index_buffer = renderer->index_buffer;
	RendererConstantBuffer* constant_buffer = renderer->vertex_constant_buffer;

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

	RendererCommitVertexMemory(vertex_buffer, vertices, ArrayCount(vertices));

	Index indices[] = {
		0,2,1, 2,3,1,
		4,5,7, 4,7,6,
		8,10,9, 10,11,9,
		12,13,15, 12,15,14,
		16,17,18, 18,17,19,
		20,23,21, 20,22,23 
	};

	RendererCommitIndexMemory(index_buffer, indices, ArrayCount(indices));

	DirectX::XMMATRIX transform = DXGenTransform(cube_data, renderer->projection);
	DirectX::XMMATRIX mvp = ApplyViewProjection(transform, renderer);
	PerObjectConstants constants = {DirectX::XMMatrixTranspose(transform), mvp};
	u32 const_buff_offset = RendererCommitConstantObjectMemory(constant_buffer, &constants, sizeof(PerObjectConstants));

	if (cube_data.texture_id != TexID_NoTexture) {
		RendererLoadTexture(renderer, cube_data.texture_id);
	}

	u32 vert_count = ArrayCount(vertices);
	u32 index_count = ArrayCount(indices);
	RenderCommand command = {
		.vertex_count = ArrayCount(vertices),
		.vertex_buffer_offset = vertex_buffer->vertex_count - vert_count, 
		.index_count = ArrayCount(indices),
		.index_buffer_offset = index_buffer->index_count - index_count,
		.vertex_constant_buffer_offset = const_buff_offset,
		.topology = RenderTopology_TriangleList,
		.texture_id = cube_data.texture_id,
	};
	PushRenderCommand(renderer->command_buffer, command);
}

void RendererPushCube(RendererState* renderer, BasicMesh cube_data) {
	RendererVertexBuffer* vertex_buffer = renderer->vertex_buffer;
	RendererIndexBuffer* index_buffer = renderer->index_buffer;
	RendererConstantBuffer* constant_buffer = renderer->vertex_constant_buffer;

	Vertex vertices[] = {
		{ {-1.0f, -1.0f, -1.0f}, COLOR_RED     },
		{ { 1.0f, -1.0f, -1.0f}, COLOR_BLUE    },
		{ {-1.0f,  1.0f, -1.0f}, COLOR_GREEN   },
		{ { 1.0f,  1.0f, -1.0f}, COLOR_YELLOW  },
		{ {-1.0f, -1.0f,  1.0f}, COLOR_MAGENTA },
		{ { 1.0f, -1.0f,  1.0f}, COLOR_CYAN    },
		{ {-1.0f,  1.0f,  1.0f}, COLOR_BLACK   },
		{ { 1.0f,  1.0f,  1.0f}, COLOR_WHITE   },
	};

	RendererCommitVertexMemory(vertex_buffer, vertices, ArrayCount(vertices));

	Index indices[] = {
		0,2,1, 2,3,1,
		1,3,5, 3,7,5,
		2,6,3, 3,6,7,
		4,5,7, 4,7,6,
		0,4,2, 2,4,6,
		0,1,4, 1,5,4 
	};

	RendererCommitIndexMemory(index_buffer, indices, ArrayCount(indices));

	DirectX::XMMATRIX transform = DXGenTransform(cube_data, renderer->projection);
	DirectX::XMMATRIX mvp = ApplyViewProjection(transform, renderer);
	PerObjectConstants constants = {DirectX::XMMatrixTranspose(transform), mvp};
	u32 const_buff_offset = RendererCommitConstantObjectMemory(constant_buffer, &constants, sizeof(PerObjectConstants));

	if (cube_data.texture_id != TexID_NoTexture) {
		RendererLoadTexture(renderer, cube_data.texture_id);
	}

	u32 vert_count = ArrayCount(vertices);
	u32 index_count = ArrayCount(indices);
	RenderCommand command = {
		.vertex_count = ArrayCount(vertices),
		.vertex_buffer_offset = vertex_buffer->vertex_count - vert_count, 
		.index_count = ArrayCount(indices),
		.index_buffer_offset = index_buffer->index_count - index_count,
		.vertex_constant_buffer_offset = const_buff_offset,
		.topology = RenderTopology_TriangleList,
		.texture_id = cube_data.texture_id,
	};
	PushRenderCommand(renderer->command_buffer, command);
}

void RendererPushCone(RendererState* renderer, BasicMesh cone) {
	u32 num_verts = cone.scale.divisions + 1 + cone.scale.divisions * 3;
	u32 num_indices = cone.scale.divisions * 6;

	RendererVertexBuffer* vertex_buffer = renderer->vertex_buffer;
	RendererIndexBuffer* index_buffer = renderer->index_buffer;
	RendererConstantBuffer* constant_buffer = renderer->vertex_constant_buffer;

	Vertex* vertices = PushArray(&renderer->scratch_storage, num_verts, Vertex);
	Index* indices = PushArray(&renderer->scratch_storage, num_indices, Index);
	Vec3 root_pos = {};
	u32 num_verts_made_base = RendererGenCircularBaseVertices(renderer, &cone, root_pos, vertices);
	Vec3 peak_pos = {root_pos.x, root_pos.y+cone.scale.height, root_pos.z};
	u32 num_verts_made_uprights = RendererGenConicalUprightVertices(renderer, &cone, peak_pos, vertices+num_verts_made_base);
	RendererCommitVertexMemory(vertex_buffer, vertices, num_verts);

	u32 num_base_indices_made = RendererGenCircularBaseIndices(renderer, &cone, 0, 1, indices, false);
	RendererGenConicalUprightIndices(renderer, &cone, num_verts_made_base, indices+num_base_indices_made);
	RendererCommitIndexMemory(index_buffer, indices, num_indices);

	if (cone.texture_id != TexID_NoTexture) {
		RendererLoadTexture(renderer, cone.texture_id);
	}

 //   Mat4 transform = RendererGenBasicMeshTransform(cone, false);
	//Mat4 inv_trans = Mat4Transpose(Mat4Inverse(transform));
	//PerObjectConstants constants = {transform, inv_trans};
	//u32 const_buff_offset = RendererCommitConstantObjectMemory(constant_buffer, &constants, sizeof(PerObjectConstants));

	////NOTE: doing this because ArrayCount was causing ull to u32 narrowing warning
	//RenderCommand command = {
	//	.vertex_count = num_verts,
	//	.vertex_buffer_offset = vertex_buffer->vertex_count - num_verts, 
	//	.index_count = num_indices,
	//	.index_buffer_offset = index_buffer->index_count - num_indices,
	//	.vertex_constant_buffer_offset = const_buff_offset,
	//	.topology = RenderTopology_TriangleList,
	//	.texture_id = cone.texture_id,
	//};
	//PushRenderCommand(renderer->command_buffer, command);
}

void RendererPushPyramid(RendererState* renderer, BasicMesh mesh) {
	RendererPushCone(renderer, mesh);
}

void RendererPushCylinder(RendererState* renderer, BasicMesh cyl) {
	u32 num_verts = cyl.scale.divisions * 2 + 2 + cyl.scale.divisions * 4;
	u32 num_indices = cyl.scale.divisions * 12; //NOTE: div*3 top, div*3 bot, div*6 sides

	RendererVertexBuffer* vertex_buffer = renderer->vertex_buffer;
	RendererIndexBuffer* index_buffer = renderer->index_buffer;
	RendererConstantBuffer* constant_buffer = renderer->vertex_constant_buffer;

	Vertex* vertices = PushArray(&renderer->scratch_storage, num_verts, Vertex);
	Index* indices = PushArray(&renderer->scratch_storage, num_indices, Index);
	Vec3 root_pos = {};
	u32 num_base_verts_made = RendererGenCircularBaseVertices(renderer, &cyl, root_pos, vertices);
	Vec3 top_center_pos = {root_pos.x, root_pos.y + cyl.scale.height, root_pos.z};
	u32 num_top_verts_made = RendererGenCircularBaseVertices(renderer, &cyl, top_center_pos, vertices+num_base_verts_made, true);
	u32 num_upright_verts_made = RendererGenCylindricalUprightVertices(renderer, &cyl, root_pos, vertices+num_base_verts_made+num_top_verts_made);
	RendererCommitVertexMemory(vertex_buffer, vertices, num_verts);

	u32 num_base_indices_made = RendererGenCircularBaseIndices(renderer, &cyl, 0, 1, indices, false);
	u32 num_top_indices_made = RendererGenCircularBaseIndices(renderer, &cyl, num_base_verts_made,
		                                                      num_base_verts_made+1, indices+num_base_indices_made, true);
	u32 num_wrapped_indices_made = RendererGenCircularWrapIndices(renderer, &cyl, num_base_verts_made+num_top_verts_made,
		                                                          indices+num_base_indices_made+num_top_indices_made);
	RendererCommitIndexMemory(index_buffer, indices, num_indices);

	if (cyl.texture_id != TexID_NoTexture) {
		RendererLoadTexture(renderer, cyl.texture_id);
	}

 //   Mat4 transform = RendererGenBasicMeshTransform(cyl, false);
	//Mat4 inv_trans = Mat4Transpose(Mat4Inverse(transform));
	//PerObjectConstants constants = {transform, inv_trans};
	//u32 const_buff_offset = RendererCommitConstantObjectMemory(constant_buffer, &constants, sizeof(PerObjectConstants));

	////NOTE: doing this because ArrayCount was causing ull to u32 narrowing warning
	//RenderCommand command = {
	//	.vertex_count = num_verts,
	//	.vertex_buffer_offset = vertex_buffer->vertex_count - num_verts, 
	//	.index_count = num_indices,
	//	.index_buffer_offset = index_buffer->index_count - num_indices,
	//	.vertex_constant_buffer_offset = const_buff_offset,
	//	.topology = RenderTopology_TriangleList,
	//	.texture_id = cyl.texture_id,
	//};
	//PushRenderCommand(renderer->command_buffer, command);

}

void RendererPushAsset(RendererState* renderer, BasicMesh mesh) {
	AssetData* asset = AssetHashMapGet(&renderer->loaded_assets, mesh.asset_id);
	if (!asset) {
		AssetData new_asset = {.id = mesh.asset_id};
		LoadDOF(&renderer->permanent_storage, &new_asset, RendererLookupAsset(renderer, mesh.asset_id));
		asset = AssetHashMapInsert(&renderer->loaded_assets, new_asset);
	}

	RendererVertexBuffer* vertex_buffer = renderer->vertex_buffer;
	RendererIndexBuffer* index_buffer = renderer->index_buffer;
	RendererConstantBuffer* constant_buffer = renderer->vertex_constant_buffer;

	RendererCommitVertexMemory(vertex_buffer, asset->vertices, asset->vertex_count);
	RendererCommitIndexMemory(index_buffer, asset->indices, asset->index_count);

	//Mat4 transform = RendererGenBasicMeshTransform(mesh);
	//Mat4 inv_trans = Mat4Transpose(Mat4Inverse(transform));
	//PerObjectConstants constants = {transform, inv_trans};
	//u32 const_buff_offset = RendererCommitConstantObjectMemory(constant_buffer, &constants, sizeof(PerObjectConstants));

	//if (mesh.texture_id != TexID_NoTexture) {
	//	RendererLoadTexture(renderer, mesh.texture_id);
	//}

	//RenderCommand command = {
	//	.vertex_count = asset->vertex_count,
	//	.vertex_buffer_offset = vertex_buffer->vertex_count - asset->vertex_count,
	//	.index_count = asset->index_count,
	//	.index_buffer_offset = index_buffer->index_count - asset->index_count,
	//	.vertex_constant_buffer_offset = const_buff_offset,
	//	.topology = RenderTopology_TriangleList,
	//	.texture_id = mesh.texture_id,
	//};
	//PushRenderCommand(renderer->command_buffer, command);
}

void RendererPushGrid(RendererState* renderer, f32 width, f32 depth, u32 rows, u32 cols, BasicMesh grid) {

	RendererVertexBuffer* vertex_buffer = renderer->vertex_buffer;
	RendererIndexBuffer* index_buffer = renderer->index_buffer;
	RendererConstantBuffer* constant_buffer = renderer->vertex_constant_buffer;

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
			vertices[i*cols+j].color = grid.color;
			//vertices[i*rows+j].tangent_u = {1.0f, 0.0f, 0.0f};
			vertices[i*cols+j].tex_coord.x = j*du;
			vertices[i*cols+j].tex_coord.y = i*dv;
		}
	}
	RendererCommitVertexMemory(vertex_buffer, vertices, vertex_count);

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
	RendererCommitIndexMemory(index_buffer, indices, index_count);

	Mat4 transform = RendererGenBasicMeshTransform(grid, false);
	Mat4 inv_trans = Mat4Transpose(Mat4Inverse(transform));
	//PerObjectConstants constants = {transform, inv_trans};
	//u32 const_buff_offset = RendererCommitConstantObjectMemory(constant_buffer, &constants, sizeof(PerObjectConstants));

	//RenderCommand command = {
	//	.vertex_count = vertex_count,
	//	.vertex_buffer_offset = vertex_buffer->vertex_count - vertex_count,
	//	.index_count = index_count,
	//	.index_buffer_offset = index_buffer->index_count - index_count,
	//	.vertex_constant_buffer_offset = const_buff_offset,
	//	.topology = RenderTopology_TriangleList,
	//	.texture_id = grid.texture_id
	//};
	//PushRenderCommand(renderer->command_buffer, command);
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
	RendererCommitConstantFrameMemory(renderer->vertex_constant_buffer, &renderer->per_frame_constants, sizeof(PointLight));
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
    RendererConstantBufferClear(renderer_state->vertex_constant_buffer);
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
