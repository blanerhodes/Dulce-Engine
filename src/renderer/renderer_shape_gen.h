#pragma once

/*TODO----------------------
 - remove duplicate vertices from cylinder generation since they can share normals between faces
 - remove duplicate vertices from cone generation since they can share normals between faces
 - change cone normal generation to point directly out from cone center instead of the face
*/

Mat4 RendererGenBasicMeshTransform(BasicMesh mesh, b32 mult_with_scale = true) {
    Mat4 trans_to_true_pos = Mat4Translation(mesh.position);
    Mat4 rotate = Mat4EulerXyz(mesh.rotation_angles.x, mesh.rotation_angles.y, mesh.rotation_angles.z);

    Mat4 transform = Mat4Identity();
	if (mult_with_scale) {
		Mat4 scale = Mat4Scale(mesh.scale);
		transform = Mat4Mult(scale, transform);
	}
    transform = Mat4Mult(rotate, transform);
    transform = Mat4Mult(trans_to_true_pos, transform);

	return transform;
}

u32 RendererGenCircularBaseVertices(RendererState* renderer, BasicMesh* mesh, Vec3 center, Vertex* vertices, f32 invert_normal = false) {
	u32 num_verts = mesh->scale.divisions + 1;
	f32 arc_offset = 0;
	
	arc_offset = -2 * PI / mesh->scale.divisions;
	Vec3 normal = {0, -1, 0};
	if (invert_normal) {
		normal.y = 1;
	}
	*vertices = {
		.position = center,
		.color = mesh->color,
		.tex_coord = {0.0f, 0.0f},
		.normal = normal
	};
	Vertex* vert_iter = vertices+1;
	for (u32 i = 0; i < mesh->scale.divisions; i++) {
		Vertex vert = {
			.position ={mesh->scale.radius*dcos(i*arc_offset), center.y, mesh->scale.radius*dsin(i*arc_offset)},
			.color = mesh->color,
			.tex_coord = {0.0f, 0.0f},
			.normal = normal
		};
		*vert_iter = vert;
		vert_iter++;
	}
	return num_verts;
}

u32 RendererGenCircularBaseIndices(RendererState* renderer, BasicMesh* mesh, u32 center_point_index, u32 start_index, Index* indices, b32 clockwise = true) {
	u32 num_indices = mesh->scale.divisions * 3;
	Index* iter = indices;
	u16 center_index = (u16)center_point_index;
	if (clockwise) {
		for (u16 base_vert = start_index; base_vert < mesh->scale.divisions+start_index-1; base_vert++) {
			*iter = {center_index};
			iter++;
			u16 second_vert = base_vert + 1;
			*iter = {second_vert};
			iter++;
			u16 third_vert = base_vert;
			*iter = {third_vert};
			iter++;
		}
		*iter = {center_index};
		iter++;
		u16 second_vert = (u16)start_index;
		*iter = {second_vert};
		iter++;
		u16 last_vert = start_index + mesh->scale.divisions-1; 
		*iter = {last_vert};
		iter++;
	}
	else {
		for (u16 base_vert = start_index; base_vert < mesh->scale.divisions+start_index-1; base_vert++) {
			*iter = {center_index};
			iter++;
			u16 second_vert = base_vert;
			*iter = {second_vert};
			iter++;
			u16 third_vert = base_vert + 1;
			*iter = {third_vert};
			iter++;
		}
		*iter = {center_index};
		iter++;
		u16 second_vert = start_index + mesh->scale.divisions-1; 
		*iter = {second_vert};
		iter++;
		u16 last_vert = (u16)start_index;
		*iter = {last_vert};
		iter++;

	}
	
	return num_indices;
}

//NOTE: vertices will already be offset from the start of the larger vertices buffer
u32 RendererGenConicalUprightVertices(RendererState* renderer, BasicMesh* mesh, Vec3 peak_pos, Vertex* vertices) {
	u32 num_verts = mesh->scale.divisions * 3;
	Vec3 center_base = {peak_pos.x, peak_pos.y - mesh->scale.height, peak_pos.z};
	f32 arc_offset = 0;
	
	arc_offset = -2 * PI / mesh->scale.divisions;

	Vertex* vert_iter = vertices;
	for (u32 vert = 0; vert < mesh->scale.divisions-1; vert++) {
		Vec3 position1 = { mesh->scale.radius * dcos((vert + 1) * arc_offset), center_base.y, mesh->scale.radius * dsin((vert + 1) * arc_offset) };
		Vec3 position2 = { mesh->scale.radius * dcos(vert * arc_offset),       center_base.y, mesh->scale.radius * dsin(vert * arc_offset) };
		Vec3 vert_to_peak = peak_pos - position1;
		Vec3 vert_to_next_vert = position2 - position1;
		Vec3 face_normal = CrossProd(vert_to_peak, vert_to_next_vert);

		*vert_iter = {
			.position = peak_pos,
			.color = mesh->color,
			.tex_coord = {0, 0},
			.normal = face_normal
		};
		vert_iter++;
		*vert_iter = {
			.position = position1,
			.color = mesh->color,
			.tex_coord = {0, 0},
			.normal = face_normal
		};
		vert_iter++;
		*vert_iter = {
			.position = position2,
			.color = mesh->color,
			.tex_coord = {0, 0},
			.normal = face_normal
		};
		vert_iter++;
	}

	Vec3 position1 = vertices[2].position;
	Vec3 position2 = (vert_iter-2)->position;
	Vec3 vert_to_peak = peak_pos - position1;
	Vec3 vert_to_next_vert = position2 - position1;
	Vec3 face_normal = CrossProd(vert_to_peak, vert_to_next_vert);

	*vert_iter = {
		.position = peak_pos,
		.color = mesh->color,
		.tex_coord = {0, 0},
		.normal = face_normal
	};
	vert_iter++;
	*vert_iter = {
		.position = position1,
		.color = mesh->color,
		.tex_coord = {0, 0},
		.normal = face_normal
	};
	vert_iter++;
	*vert_iter = {
		.position = position2,
		.color = mesh->color,
		.tex_coord = {0, 0},
		.normal = face_normal
	};
	
	return num_verts;

}

u32 RendererGenConicalUprightIndices(RendererState* renderer, BasicMesh* mesh, u32 start_index, Index* indices, b32 clockwise = true) {
	u32 num_indices = mesh->scale.divisions*3;
	for (u32 i = 0; i < num_indices; i++) {
		(indices+i)->value = start_index+i;
	}
	return num_indices;
}

u32 RendererGenCircularWrapIndices(RendererState* renderer, BasicMesh* mesh, u32 start_index, Index* indices) {
	u32 indices_per_div = 6;
	u32 num_indices = mesh->scale.divisions * indices_per_div;
	for (u32 face = 0; face < mesh->scale.divisions; face++) {
		u32 offset = face * 4;
		indices->value = start_index + offset;
		indices++;
		indices->value = start_index + offset + 1;
		indices++;
		indices->value = start_index + offset + 2;
		indices++;
		indices->value = start_index + offset + 2;
		indices++;
		indices->value = start_index + offset + 3;
		indices++;
		indices->value = start_index + offset;
		indices++;
	}
	return num_indices;
}

u32 RendererGenCylindricalUprightVertices(RendererState* renderer, BasicMesh* mesh, Vec3 base_center, Vertex* vertices) {
	u32 num_verts = mesh->scale.divisions * 4;
	Vec3 top_center = base_center;
	top_center.y += mesh->scale.height;
	f32 arc_offset = 0;

	arc_offset = -2 * PI / mesh->scale.divisions;

	Vertex* vert_iter = vertices;
	for (u32 vert = 0; vert < mesh->scale.divisions-1; vert++) {
		//NOTE: left and right are relative to looking at the face head on
		f32 leftx = mesh->scale.radius * dcos((vert + 1) * arc_offset);
		f32 leftz = mesh->scale.radius * dsin((vert + 1) * arc_offset);
		f32 rightx = mesh->scale.radius * dcos(vert * arc_offset);
		f32 rightz = mesh->scale.radius * dsin(vert * arc_offset);
		Vec3 position1 = {leftx, base_center.y, leftz};
		Vec3 position2 = {leftx, top_center.y, leftz};
		Vec3 position3 = {rightx, top_center.y, rightz};
		Vec3 position4 = {rightx, base_center.y, rightz};
		Vec3 pos1_normal = position1 - base_center;
		Vec3 pos2_normal = position2 - top_center;
		Vec3 pos3_normal = position3 - top_center;
		Vec3 pos4_normal = position4 - base_center;

		*vert_iter = {
			.position = position4,
			.color = mesh->color,
			.tex_coord = {0, 0},
			.normal = pos4_normal
		};
		vert_iter++;
		*vert_iter = {
			.position = position3,
			.color = mesh->color,
			.tex_coord = {0, 0},
			.normal = pos3_normal
		};
		vert_iter++;
		*vert_iter = {
			.position = position2,
			.color = mesh->color,
			.tex_coord = {0, 0},
			.normal = pos2_normal
		};
		vert_iter++;
		*vert_iter = {
			.position = position1,
			.color = mesh->color,
			.tex_coord = {0, 0},
			.normal = pos1_normal
		};
		vert_iter++;
	}

	Vec3 position1 = (vert_iter-1)->position;
	Vec3 position2 = (vert_iter-2)->position;
	Vec3 position3 = vertices[1].position;
	Vec3 position4 = vertices[0].position;
	Vec3 pos1_normal = position1 - base_center;
	Vec3 pos2_normal = position2 - top_center;
	Vec3 pos3_normal = position3 - top_center;
	Vec3 pos4_normal = position4 - base_center;

	*vert_iter = {
		.position = position1,
		.color = mesh->color,
		.tex_coord = {0, 0},
		.normal = pos1_normal
	};
	vert_iter++;
	*vert_iter = {
		.position = position2,
		.color = mesh->color,
		.tex_coord = {0, 0},
		.normal = pos2_normal
	};
	vert_iter++;
	*vert_iter = {
		.position = position3,
		.color = mesh->color,
		.tex_coord = {0, 0},
		.normal = pos3_normal
	};
	vert_iter++;
	*vert_iter = {
		.position = position4,
		.color = mesh->color,
		.tex_coord = {0, 0},
		.normal = pos4_normal
	};
	
	return num_verts;
}
