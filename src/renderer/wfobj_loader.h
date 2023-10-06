#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>

//TODO: some way to reorder the indices so accessing them is more gpu vertex cache friendly

Vec3 WFGetTriplets(std::istringstream& stream) {
	Vec3 triplets = {};
	stream >> triplets.x >> triplets.y >> triplets.z;
	return triplets;
}

Vec3 WFGetIndices(std::istringstream& stream) {
	Vec3 triplets = WFGetTriplets(stream);
	return {triplets.x-1, triplets.y-1, triplets.z-1};
}

void WFProcessVertex(Vertex vertex, std::unordered_map<Vertex, u32>& unique_verts, 
	                 std::vector<Vertex>& vertices, std::vector<Index>indices, u32* index_counter) {
		auto vert_loc = unique_verts.find(vertex);
		if (vert_loc == unique_verts.end()) {
			std::pair<Vertex, u32> pair = {vertex, *index_counter};
			unique_verts.insert(pair);
			vertices.push_back(vertex);
			u16 counter16 = (u16)(*index_counter);
			Index temp = {counter16};
			indices.push_back(temp);
		}
		else {
			Index temp;
			temp.value = vert_loc->second;
			indices.push_back(temp);
		}
		*(index_counter)++;
}

//NOTE: wavefront obj to dulce object file
void WFObjToDof(char* in_file, char* out_file) {
	u32 buff_size = 256;
	std::string buffer;
	std::vector<Vec3> positions;
	std::vector<Vec3> normals;
	std::vector<Vec2> tex_coords;

	std::ifstream file(in_file);
	std::istringstream stream(buffer);
	u32 skip_lines = 4;
	for (u32 i = 0; i < skip_lines; i++) {
		std::getline(file, buffer);
	}
	
	//NOTE: fill pos/norm/tex arrays with raw data
	while (std::getline(file, buffer)) {
		stream.str(buffer);
		std::string indicator;
		stream >> indicator;
		
		if (indicator == "v") {
			positions.push_back(WFGetTriplets(stream));
		}
		else if (indicator == "vn") {
			normals.push_back(WFGetTriplets(stream));
		}
		else if (indicator == "vt") {
			Vec2 uv = {};
			stream >> uv.x >> uv.y;
			tex_coords.push_back(uv);
		}
		else if (indicator == "f") {
			break;
		}
	}


	std::unordered_map<Vertex, u32> unique_verts;
	std::vector<Vertex> vertices;
	std::vector<Index> indices;
	u32 index_counter = 0;
	do {
		stream.str(buffer);
		char dump;
		stream >> dump;
		//NOTE: get adjusted indices from the arrays made from the file
		Vec3 vert1 = WFGetIndices(stream);
		Vec3 vert2 = WFGetIndices(stream);
		Vec3 vert3 = WFGetIndices(stream);

		Vertex vertex1 = {
			.position = positions[vert1.x],
			.color = {1, 1, 1},
			.tex_coord = tex_coords[vert1.y],
			.normal = normals[vert1.z]
		};
		Vertex vertex2 = {
			.position = positions[vert2.x],
			.color = {1, 1, 1},
			.tex_coord = tex_coords[vert2.y],
			.normal = normals[vert2.z]
		};
		Vertex vertex3 = {
			.position = positions[vert3.x],
			.color = {1, 1, 1},
			.tex_coord = tex_coords[vert3.y],
			.normal = normals[vert3.z]
		};
		//NOTE: swapped order for CW winding
		WFProcessVertex(vertex1, unique_verts, vertices, indices, &index_counter);
		WFProcessVertex(vertex3, unique_verts, vertices, indices, &index_counter);
		WFProcessVertex(vertex2, unique_verts, vertices, indices, &index_counter);
	
	} while (std::getline(file, buffer));

	std::ofstream dof(out_file, std::ios::binary);
	dof.write((char*)vertices.size(), sizeof(u32));
	dof.write((char*)indices.size(), sizeof(u32));
	for (u32 i = 0; i < vertices.size(); i++) {
		dof.write((char*)&vertices[i], sizeof(vertices[0]));
	}
	for (u32 i = 0; i < indices.size(); i++) {
		dof.write((char*)&indices[i], sizeof(indices[0]));
	}
	dof.close();
}

 void LoadDOF(MemoryArena* arena, AssetData* asset, char* filename) {
	DebugReadFileResult read_result = DebugPlatformReadEntireFile(0, filename);
	u32 vert_count = 0;
	u32 index_count = 0;
	u32* counts = (u32*)read_result.contents;
	vert_count = *counts;
	counts++;
	index_count = *counts;
	counts++;

	asset->vertices = PushArray(arena, vert_count, Vertex);
	Vertex* read_vertices = (Vertex*)counts;
	Vertex* write_vertices = asset->vertices;
	MemCopy(read_vertices, write_vertices, vert_count*sizeof(Vertex));

	asset->indices = PushArray(arena, index_count, Index);
	Index* read_indices = (Index*)read_vertices;
	Index* write_indices = asset->indices;
	MemCopy(read_indices, write_indices, index_count*sizeof(Index));

	DebugPlatformFreeFileMemory(0, read_result.contents);
}

 void LoadAssetConfig(RendererState* renderer) {
	 char* filename = "../../resources/assets/assets.ini";
	 DebugReadFileResult read_result = DebugPlatformReadEntireFile(0, filename);
	 renderer->assets_table = (AssetID*)PushSize(&renderer->permanent_storage, 0);
	 u8* file_ptr = (u8*)read_result.contents;
	 u32 num_copied = 0;
	 AssetID curr = {};
	 while (num_copied = StringCopyToWS(file_ptr, curr.name, true)) {
		file_ptr += num_copied;
		num_copied = StringCopyToWS(file_ptr, curr.filepath, true);
		file_ptr += num_copied;
		AssetID* next_id = PushStruct(&renderer->permanent_storage, AssetID);
		MemCopy(&curr, next_id, sizeof(AssetID));
	 }
 }


//NOTE: .dof format everything is tightly packed
//vert_count
//index_count
//all vertices
//all indices
