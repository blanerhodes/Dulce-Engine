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

cbuffer cbPerObject : register(b1) {
   float4x4 model_transform;
   float4x4 model_view_proj;
};

struct VertIn {
    float3 position : Position;
    float4 color : Color;
    float2 tex_coord : TexCoord;
    float3 normal : Normal;
};

struct VertOut {
    float4 world_pos : Position;
    float4 color : Color;
    float2 tex_coord : TexCoord;
    float3 normal : Normal;
	float4 pos : SV_POSITION;
};

VertOut main(VertIn vin) {
    VertOut vs_out;
    vs_out.world_pos = mul(float4(vin.position, 1.0f), model_transform);
	vs_out.color = vin.color;
    vs_out.tex_coord = vin.tex_coord;
	vs_out.pos = mul(float4(vin.position, 1.0f), model_view_proj);
    return vs_out;
}
