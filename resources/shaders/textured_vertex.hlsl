struct PointLight {
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float3 position;
	float range;
	float3 attenuation;
	float pad;
};

struct DirectionalLight {
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float3 direction;
	float pad;
};

struct SpotLight {
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float3 position;
	float range;
	float3 direction;
	float spot;
	float3 attenuation;
	float pad;
};

struct Material {
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 reflect;
};

cbuffer cbPerFrame : register(b0) {
    row_major float4x4 proj_view;
    row_major float4x4 norm_transform;
    float4 light_pos;
    float4 light_color;
};

//cbuffer cbPerObject {
//    float4x4 g_world;
//    float4x4 g_world_inv_transpose;
//    float4x4 g_world_view_proj;
//    Material g_material;
//};
//
//cbuffer cbPerFrame : register(b2) {
//    DirectionalLight g_dir_light;
//    PointLight g_point_light;
//    SpotLight g_spot_light;
//    float3 g_eye_pos_w;
//};

cbuffer cbPerObject : register(b1) {
   row_major matrix model_transform;
   row_major matrix inv_trans;
};

struct VertIn {
    float3 position : POS;
    float4 color : COL;
    float2 tex_coord : TEX;
    float3 normal : NOR;
};

struct VertOut {
    float4 position : SV_POSITION;
    float4 color : COL;
    float2 tex_coord : TEX;
    float3 normal : NOR;
    float3 frag_pos : FP;
    float3 light_pos : LP;
    //float4 light_color : LC;
};

VertOut main(VertIn input) {
    //process UI stuff if normal is all 0s
    float4x4 mvp = mul(proj_view, model_transform);
    VertOut output;
    output.normal = mul((float3x3) inv_trans, input.normal);
    output.position = mul(mvp, float4(input.position, 1.0f));
    output.frag_pos = output.position; 
    output.color = input.color;
    output.tex_coord = input.tex_coord;
    output.light_pos = light_pos;
    return output;
}

//VertOut main(VertIn input) {
   // matrix identity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
   // float ambient_stength = 0.1f;

 //   matrix transposed_transform = model_transform;
 //   float4 corrected_normal = normalize(mul(transposed_transform, float4(input.normal, 0.0f)));
 //   float4 corrected_vert_pos = mul(model_transform, float4(input.position, 1.0f));
 //   float4 distance_to_light = distance(corrected_vert_pos, light_pos);
 //   //NOTE: this 50 is a hardcoded value that should be replaced with a distance the light can traval
 //   float intensity_falloff = 1.0f - distance_to_light / 80.0f; 

 //   float3 light_direction = normalize(corrected_vert_pos - light_pos);
 //   float incident_angle = max(0.0f, dot(corrected_normal, -light_direction));
 //   float4 corrected_light_color = float4(light_color.x, light_color.y, light_color.z, 1.0f) * (light_color.w * intensity_falloff);
 //   float4 corrected_color = input.color * corrected_light_color * incident_angle;

 //   //float4 frag_pos = mul(model_transform, input.position);

 //   matrix mvp = mul(proj_view, model_transform);

 //   VertOut output;
 //   output.position = mul(mvp, float4(input.position, 1.0f));
 //   output.color = corrected_color;//float4(light_direction, 1.0f);
 //   //output.color = input.color;
 //   output.tex_coord = input.tex_coord;
 //   output.normal = corrected_normal;
 //   return output;
//}