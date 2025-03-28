Texture2D tex : register(t0); 

SamplerState splr : register(s0);

struct PixIn {
    float4 position : SV_POSITION;
    float4 color : COL;
    float2 tex_coord : TEX;
    float3 normal : NOR;
    float3 frag_pos : FP;
    float3 light_pos : LP;
    //float4 light_color : LC;
};

static float3 ambient = { 0.15, 0.15, 0.15 };
static float3 diffuse_color = { 1.0f, 1.0f, 1.0f };
static float diffuse_intensity = 1.0f;
static float att_const = 1.0f;
static float att_lin = 1.0f;
static float att_quad = 1.0f;

/*
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
*/

float4 main(PixIn input) : SV_Target{
    float3 light_dir = input.light_pos - input.frag_pos;
    float3 light_distance = length(light_dir);
    float3 light_dir_norm = light_dir / light_distance;
    
    float3 corrected_normal = normalize(input.normal);
    float attenuation = 100 / (att_const + att_lin * light_distance + att_quad * (light_distance * light_distance));
    float diffuse = diffuse_color * diffuse_intensity * attenuation * max(0.0f, dot(light_dir_norm, corrected_normal));

    //return tex.Sample(splr, input.tex_coord) * float4(result, input.color.a);
    //return tex.Sample(splr, input.tex_coord) * diffuse;
    return input.color;
}
