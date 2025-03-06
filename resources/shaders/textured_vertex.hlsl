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

cbuffer ProjView : register(b0) {
    row_major float4x4 proj_view;
    row_major float4x4 norm_transform;
    float4 light_pos;
    float4 light_color;
};

cbuffer cbPerObject {
    float4x4 g_world;
    float4x4 g_world_inv_transpose;
    float4x4 g_world_view_proj;
    Material g_material;
};

cbuffer cbPerFrame : register(b2) {
    DirectionalLight g_dir_light;
    PointLight g_point_light;
    SpotLight g_spot_light;
    float3 g_eye_pos_w;
};

cbuffer ModelTransform : register(b1) {
   row_major matrix model_transform;
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
    float3 light_direction : FP;
    float3 light_pos : LP;
    //float4 light_color : LC;
};

void ComputeDirectionalLight(Material mat, DirectionalLight light, float3 normal, float3 to_eye, out float4 ambient, out float4 diffuse, out float4 spec) {
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float3 light_vec = -light.direction;
    ambient = mat.ambient * light.ambient;

    float diffuse_factor = dot(light_vec, normal);
    [flatten]
    if (diffuse_factor > 0.0f) {
        float3 v = reflect(-light_vec, normal);
        float p = mat.specular.w;
        float spec_factor = pow(max(dot(v, to_eye), 0.0f), p);
        diffuse = diffuse_factor * mat.diffuse * light.diffuse;
        spec = spec_factor * mat.specular * light.specular;
    }
}

void ComputePointLight(Material mat, PointLight light, float3 pos, float3 normal, float3 to_eye, out float4 ambient, out float4 diffuse, out float4 spec) {
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float3 light_vec = light.position - pos;
    float d = length(light_vec);
    if (d > light.range) {
        return;
    }
    light_vec /= d;
    ambient = mat.ambient * light.ambient;

    float diffuse_factor = dot(light_vec, normal);
    [flatten]
    if (diffuse_factor > 0.0f) {
        float3 v = reflect(-light_vec, normal);
        float p = mat.specular.w;
        float spec_factor = pow(max(dot(v, to_eye), 0.0f), p);
        diffuse = diffuse_factor * mat.diffuse * light.diffuse;
        spec = spec_factor * mat.specular * light.specular;
    }

    float att = 1.0f / dot(light.attenuation, float3(1.0f, d, d*d));
    diffuse *= att;
    spec *= att;
}

void ComputeSpotLight(Material mat, SpotLight light, float3 pos, float3 normal, float3 to_eye, out float4 ambient, out float4 diffuse, out float4 spec) {
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float3 light_vec = light.position - pos;
    float d = length(light_vec);
    if (d > light.range) {
        return;
    }
    light_vec /= d;
    ambient = mat.ambient * light.ambient;

    float diffuse_factor = dot(light_vec, normal);
    [flatten]
    if (diffuse_factor > 0.0f) {
        float3 v = reflect(-light_vec, normal);
        float p = mat.specular.w;
        float spec_factor = pow(max(dot(v, to_eye), 0.0f), p);
        diffuse = diffuse_factor * mat.diffuse * light.diffuse;
        spec = spec_factor * mat.specular * light.specular;
    }

    float spot = pow(max(dot(-light_vec, light.direction), 0.0f), light.spot);
    ambient *= spot;

    float att = spot / dot(light.attenuation, float3(1.0f, d, d*d));
    diffuse *= att;
    spec *= att;
}

VertOut main(VertIn input) {
    //process UI stuff if normal is all 0s
    float4x4 mvp = mul(proj_view, model_transform);
    VertOut output;
    output.normal = mul((float3x3) model_transform, input.normal);
    output.position = mul(mvp, float4(input.position, 1.0f));
    output.light_direction = light_pos - output.position; 
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