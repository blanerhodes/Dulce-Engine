Texture2D tex : register(t0); 

SamplerState splr : register(s0);

struct PixIn {
    float4 world_pos : Position;
    float4 color : Color;
    float2 tex_coord : TexCoord;
    float3 normal : Normal;
	float4 pos : SV_POSITION;
};

float4 main(PixIn input) : SV_Target {
    float4 result = tex.Sample(splr, input.tex_coord); 
    return result;
}
