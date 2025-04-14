struct PixIn {
    float4 world_pos : Position;
    float4 color : Color;
    float2 tex_coord : TexCoord;
    float3 normal : Normal;
	float4 pos : SV_POSITION;
};

float4 main(PixIn ps_in) : SV_Target{
    return ps_in.color;
}
