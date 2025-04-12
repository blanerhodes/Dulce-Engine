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
    float4 pos : SV_Position;
};


float4 main(PSIn pin) : SV_Target {
    float3 v_tol = light_pos - pin.world_pos;
    float dist_tol = length(v_tol);
    float3 dir_tol = v_tol / dist_tol;
    float att = 1.0f / (att_const + att_lin * dist_tol + att_quad * (dist_tol * dist_tol));
    float3 diffuse = diffuse_color * diffuse_intensity * att * max(0.0f, dot(dir_tol, pin.normal));
    //test
    return float4(saturate((diffuse + ambient) * pin.color), 1.0f);
}