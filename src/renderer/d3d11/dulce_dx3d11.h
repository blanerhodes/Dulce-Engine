#pragma once
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <dxgidebug.h>
#include "../render.h"

struct D3DTexture2D {
    u32 id;
    ID3D11Texture2D* texture;
    ID3D11ShaderResourceView* shader_view;
};
//TODO: handle releasing this stuff for d3d's internal ref counting
struct Direct3d {
    ID3D11Device* base_device; //used for allocating stuff
    ID3D11Device1* device;
    ID3D11DeviceContext* base_context; //used for configuring pipelines and issuing draw commands
    ID3D11DeviceContext1* context; 
    IDXGIDevice1* dxgi_device;
    IDXGIAdapter* dxgi_adapter;
    IDXGIFactory2* dxgi_factory;
    IDXGISwapChain1* swap_chain;

    IDXGIInfoQueue* dxgi_debug_info;
    ID3D11InfoQueue* d3d_debug_info;

    ID3D11Texture2D* frame_buffer;
    ID3D11RenderTargetView* frame_buffer_view;
    ID3D11Texture2D* depth_buffer;
    ID3D11DepthStencilView* depth_buffer_view;
    ID3D11RasterizerState1* rasterizer_state;
    ID3D11DepthStencilState* depth_stencil_state;
    D3D11_VIEWPORT viewport;

    ID3D11Buffer* vertex_buffer;
    ID3D11Buffer* index_buffer;
    ID3D11Buffer* constant_buffer;
    ID3D11Buffer* proj_view_buffer;
    
    ID3D11VertexShader* vertex_shader;
    ID3D11PixelShader* pixel_shaders[2];
    PixelShaderType bound_pixel_shader;
    
    u32 next_free_tex_slot;
    D3DTexture2D textures_2d[16];
    ID3D11SamplerState* sampler;

    u32 view_width;
    u32 view_height;
    f32 aspect_ratio;
};