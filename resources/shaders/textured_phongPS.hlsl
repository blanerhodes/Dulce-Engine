cbuffer LightCBuf : register(b0) {
    float3 light_pos;
    float3 mat_color;
    float3 ambient;
    float3 diffuse_color;
    float diffuse_intensity;
    float att_const;
    float att_lin;
    float att_quad;
};

struct PSIn {
    float3 world_pos : Position;
    float3 color : Color;
    float3 normal : Normal;
    float2 tex_coord : TexCoord;
    float4 pos : SV_Position;
};

Texture2D tex;
SamplerState splr;

float4 main(PSIn pin) : SV_Target {
    float3 v_tol = light_pos - pin.world_pos;
    float dist_tol = length(v_tol);
    float3 dir_tol = v_tol / dist_tol;
    float att = 1.0f / (att_const + att_lin * dist_tol + att_quad * (dist_tol * dist_tol));
    float3 diffuse = diffuse_color * diffuse_intensity * att * max(0.0f, dot(dir_tol, pin.normal));
    float4 sample_color;
    if (pin.tex_coord.y > 0.75f || pin.tex_coord.y < 0.05f || pin.tex_coord.x < 0.01f || pin.tex_coord.x > 0.99f) {
        sample_color = float4(0.87f, 0.87f, 0.89f, 1.0f);
    } else {
        sample_color = tex.Sample(splr, pin.tex_coord); 
    }
    return  float4(saturate((diffuse + ambient)), 1.0f) * sample_color; 
}