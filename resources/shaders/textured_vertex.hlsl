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

cbuffer cbPerFrame : register(b0) {
    float4x4 proj_view;
    float4x4 norm_transform;
    float4 light_pos;
    float4 light_color;
};

cbuffer cbPerObject : register(b1) {
   float4x4 model_transform;
};

struct VertIn {
    float3 position : Position;
    float4 color : Color;
    float2 tex_coord : TexCoord;
    float3 normal : Normal;
};

struct VertOut {
    float4 position : SV_POSITION;
    float4 color : Color;
    float2 tex_coord : TexCoord;
    float3 normal : Normal;
    //float3 frag_pos : FP;
    //float3 light_pos : LP;
    //float4 light_color : LC;
};

VertOut main(VertIn input) {
    VertOut vs_out;
    vs_out.position = mul(float4(input.position, 1.0f), model_transform);
    vs_out.color = input.color;
    vs_out.tex_coord = input.tex_coord;
    return vs_out;
}
