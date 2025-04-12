struct VSIn {
    float3 position : Position;
    float4 color : Color;
    float2 tex_coord : TexCoord;
    float3 normal : Normal;
};

struct VSOut {
    float3 world_pos : Position;
    float3 color : Color;
    float3 normal : Normal;
    float4 pos : SV_Position;
};

cbuffer cbPerObject : register(b1) {
   float4x4 model_transform;
   float4x4 model_view_proj;
};

VSOut main(VSIn vin) {
    VSOut vso;
    vso.world_pos = (float3)mul(float4(vin.position, 1.0f), model_transform);
    vso.color = vin.color;
    vso.normal = mul(vin.normal, (float3x3)model_transform);
    vso.pos = mul(float4(vin.position, 1.0f), model_view_proj);
    //test
    return vso;
}