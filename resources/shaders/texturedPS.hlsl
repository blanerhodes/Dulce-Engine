Texture2D tex : register(t0); 

SamplerState splr : register(s0);

struct PixIn {
    float4 world_pos : Position;
    float4 color : Color;
    float2 tex_coord : TexCoord;
    float3 normal : Normal;
	float4 pos : SV_POSITION;
};

float4 main(PixIn pin) : SV_Target {
    float4 result;
    if (pin.tex_coord.y > 0.75f || pin.tex_coord.y < 0.05f || pin.tex_coord.x < 0.01f || pin.tex_coord.x > 0.99f) {
        result = float4(0.87f, 0.87f, 0.89f, 1.0f);
    } else {
        result = tex.Sample(splr, pin.tex_coord); 
    }
    return result;
}
