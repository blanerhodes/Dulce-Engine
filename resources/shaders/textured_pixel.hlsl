Texture2D tex : register(t0); 

SamplerState splr : register(s0);

struct PixIn {
    float4 position : SV_POSITION;
    float4 color : COL;
    float2 tex_coord : TEX;
    float3 normal : NOR;
    float3 ligth_direction : FP;
    float3 light_pos : LP;
    //float4 light_color : LC;
};

static float3 ambient = { 0.15, 0.15, 0.15 };
static float3 diffuse_color = { 1.0f, 1.0f, 1.0f };
static float diffuse_intensity = 1.0f;
static float att_const = 1.0f;
static float att_lin = 1.0f;
static float att_quad = 1.0f;

float4 main(PixIn input) : SV_Target{
    float3 light_dir = input.light_pos - input.frag_pos;
    float3 light_distance = length(light_dir);
    float3 light_dir_norm = light_dir / light_distance;
    
    float3 corrected_normal = normalize(input.normal);
    float attenuation = 100 / (att_const + att_lin * light_distance + att_quad * (light_distance * light_distance));
    float diffuse = diffuse_color * diffuse_intensity * attenuation * max(0.0f, dot(light_dir_norm, corrected_normal));

    //return tex.Sample(splr, input.tex_coord) * float4(result, input.color.a);
    return tex.Sample(splr, input.tex_coord) * diffuse;
}
