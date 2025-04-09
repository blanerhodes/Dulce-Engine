cbuffer LightCBuf : register(b0) {
    float3 light_pos;
};

float3 mat_color = {0.7f, 0.7f, 0.9f};
float3 ambient = {0.15f, 0.15f, 0.15f};
float3 diffuse_color = {1.0f, 1.0f, 1.0f};
float diffuse_intensity = 1.0f;
float att_const = 1.0f;
float att_lin = 0.045f;
float att_quad = 0.0075f;

float4 main(float3 world_pos : Position, float3 n : Normal) : SV_Target {
    float3 v_tol = light_pos - world_pos;
    float dist_tol = length(v_tol);
    float3 dir_tol = v_tol / dist_tol;
    float att = 1.0f / (att_const + att_lin * dist_tol + att_quad * (dist_tol * dist_tol));
    float3 diffuse = diffuse_color * diffuse_intensity * att * max(0.0f, dot(dir_tol, n));
    return float4(saturate(diffuse + ambient), 1.0f);
}