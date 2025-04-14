#include "dulce_dx3d11.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3d11sdklayers.h>
#include "defines.h"
#include "../render.h"
#include "asserts.h"
#include "../../vendor/imgui/imgui.h"
#include "../../vendor/imgui/imgui_impl_win32.h"
#include "../../vendor/imgui/imgui_impl_dx11.h"

enum RenderGeometryOffset {
    GeoOffset_Cube,
    GeoOffset_Line,
    GeoOffset_Plane,
    GeoOffset_Pyramid,
    GeoOffset_Sphere,
    GeoOffset_Terminator
};

struct D3DState {
    ID3D11Buffer* vertex_buffer;
    void* vertex_buffer_backing;
    u32 vertex_buffer_size;
    ID3D11Buffer* index_buffer;
    u32 index_buffer_size;
    ID3D11Buffer* instanced_transform;
    u32 vertex_geometry_offsets[GeoOffset_Terminator];
    u32 index_geometry_offsets[GeoOffset_Terminator];
};

typedef HRESULT DXGIGetDebugInterface_ (REFIID, void**);
static DXGIGetDebugInterface_* DxgiGetDebugInterface;

void InitDirect3D(HWND hwnd, u32 view_width, u32 view_height) {
    D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0};

    D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_DEBUG, feature_levels, ArrayCount(feature_levels), D3D11_SDK_VERSION, &g_d3d.base_device, 0, &g_d3d.base_context);
    g_d3d.base_device->QueryInterface(__uuidof(ID3D11Device1), (void**)&g_d3d.device);
    g_d3d.base_context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&g_d3d.context);

    g_d3d.device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&g_d3d.d3d_debug_info);
    g_d3d.d3d_debug_info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    g_d3d.d3d_debug_info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
    g_d3d.d3d_debug_info->Release();

    HMODULE dxgi_debug = LoadLibraryExA("dxgidebug.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    DxgiGetDebugInterface = (DXGIGetDebugInterface_*)GetProcAddress(dxgi_debug, "DXGIGetDebugInterface");
    DxgiGetDebugInterface(__uuidof(IDXGIInfoQueue), (void**)&g_d3d.dxgi_debug_info);
    g_d3d.dxgi_debug_info->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    g_d3d.dxgi_debug_info->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
    g_d3d.dxgi_debug_info->Release();

    g_d3d.device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&g_d3d.dxgi_device);
    g_d3d.dxgi_device->GetAdapter(&g_d3d.dxgi_adapter);
    g_d3d.dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), (void**)&g_d3d.dxgi_factory);

    DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
    swapchain_desc.Width = 0;
    swapchain_desc.Height = 0;
    swapchain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapchain_desc.Stereo = FALSE;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
    //If anyone is using FLIP_DISCARD / FLIP_SEQUENTIAL don't remove SetRenderTargets call from DrawTriangle.
    //Add the depthstencilview pointer instead of nullptr there. As FLIP mode removes binds every frame/flip.
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; //use this later DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchain_desc.Flags = 0;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {
        .RefreshRate = {
            .Numerator = 0,
            .Denominator = 0
        },
        .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
        .Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
        .Windowed = true
    };

    g_d3d.dxgi_factory->CreateSwapChainForHwnd(g_d3d.device, hwnd, &swapchain_desc, &fullscreen_desc, 0, &g_d3d.swap_chain);
    g_d3d.swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&g_d3d.frame_buffer);
    g_d3d.device->CreateRenderTargetView(g_d3d.frame_buffer, 0, &g_d3d.frame_buffer_view);

    D3D11_TEXTURE2D_DESC depth_buffer_desc = {
        .Width = view_width,
        .Height = view_height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_DEPTH_STENCIL
    };

    g_d3d.device->CreateTexture2D(&depth_buffer_desc, 0, &g_d3d.depth_buffer);

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
        .Format = DXGI_FORMAT_D32_FLOAT,
        .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
        .Texture2D = {
            .MipSlice = 0
        }
    };
    g_d3d.device->CreateDepthStencilView(g_d3d.depth_buffer, &dsv_desc, &g_d3d.depth_buffer_view);

    D3D11_RASTERIZER_DESC1 rd_solid = {};
    rd_solid.FillMode = D3D11_FILL_SOLID;
    rd_solid.CullMode = D3D11_CULL_BACK; 
    rd_solid.FrontCounterClockwise = false;
    rd_solid.DepthClipEnable = true;
    g_d3d.device->CreateRasterizerState1(&rd_solid, &g_d3d.solid_rs);

    D3D11_RASTERIZER_DESC1 rd_wireframe = {};
    rd_wireframe.FillMode = D3D11_FILL_WIREFRAME;
    rd_wireframe.CullMode = D3D11_CULL_FRONT; 
    rd_wireframe.FrontCounterClockwise = false;
    rd_wireframe.DepthClipEnable = true;
    g_d3d.device->CreateRasterizerState1(&rd_wireframe, &g_d3d.wireframe_rs);

    D3D11_RASTERIZER_DESC1 rd_no_cull = {};
    rd_no_cull.FillMode = D3D11_FILL_SOLID;
    rd_no_cull.CullMode = D3D11_CULL_NONE; 
    rd_no_cull.FrontCounterClockwise = false;
    rd_no_cull.DepthClipEnable = true;
    g_d3d.device->CreateRasterizerState1(&rd_no_cull, &g_d3d.no_cull_rs);

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    //sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    g_d3d.device->CreateSamplerState(&sampler_desc, &g_d3d.sampler);
    D3DBindSampler();

    D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};
    depth_stencil_desc.DepthEnable = TRUE;
    depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
    g_d3d.device->CreateDepthStencilState(&depth_stencil_desc, &g_d3d.depth_stencil_state);

    g_d3d.view_width = view_width;
    g_d3d.view_height = view_height;
    g_d3d.aspect_ratio = (f32)view_height / (f32)view_width;

    g_d3d.context->OMSetRenderTargets(1, &g_d3d.frame_buffer_view, g_d3d.depth_buffer_view);
    g_d3d.context->OMSetDepthStencilState(g_d3d.depth_stencil_state, 0);
    g_d3d.context->OMSetBlendState(0, 0, 0xFFFFFFFF); //NOTE: default blend mode

    g_d3d.viewport = {
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = (f32)depth_buffer_desc.Width,
        .Height = (f32)depth_buffer_desc.Height,
        .MinDepth = 0,
        .MaxDepth = 1
    };
    g_d3d.context->RSSetViewports(1, &g_d3d.viewport);

    ImGui_ImplDX11_Init(g_d3d.device, g_d3d.context);

    g_d3d.next_free_tex_slot = 0;

}

void D3DClearBuffer(f32 red, f32 green, f32 blue) {
    Vec4 color = {red, green, blue, 1.0f};
    g_d3d.context->ClearRenderTargetView(g_d3d.frame_buffer_view, (f32*)&color);
    g_d3d.context->ClearDepthStencilView(g_d3d.depth_buffer_view, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

D3D11_PRIMITIVE_TOPOLOGY D3DGetTopology(RenderTopology topology) {
    switch (topology) {
        case RenderTopology_TriangleList: 
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case RenderTopology_LineList:
            return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case RenderTopology_PointList:
            return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        default: {INVALID_CODE_PATH;}
    }
    return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

//NOTE: Defaulting to phong shaders
void D3DLoadShaders(RendererState* renderer) {
    ID3DBlob* blob;
    D3DReadFileToBlob(L"./shaders/untexturedPS.cso", &blob);
    g_d3d.device->CreatePixelShader(blob->GetBufferPointer(),
                                    blob->GetBufferSize(),
                                    0,
                                    &g_d3d.pixel_shaders[PixelShaderType_Untextured]);

    D3DReadFileToBlob(L"./shaders/texturedPS.cso", &blob);
    g_d3d.device->CreatePixelShader(blob->GetBufferPointer(),
                                    blob->GetBufferSize(),
                                    0,
                                    &g_d3d.pixel_shaders[PixelShaderType_Textured]);

    D3DReadFileToBlob(L"./shaders/textured_phongPS.cso", &blob);
    g_d3d.device->CreatePixelShader(blob->GetBufferPointer(),
                                    blob->GetBufferSize(),
                                    0,
                                    &g_d3d.pixel_shaders[PixelShaderType_TexturedPhong]);

    D3DReadFileToBlob(L"./shaders/phongPS.cso", &blob);
    g_d3d.device->CreatePixelShader(blob->GetBufferPointer(),
                                    blob->GetBufferSize(),
                                    0,
                                    &g_d3d.pixel_shaders[PixelShaderType_UntexturedPhong]);
    g_d3d.context->PSSetShader(g_d3d.pixel_shaders[PixelShaderType_UntexturedPhong], 0, 0);

    D3DReadFileToBlob(L"./shaders/texturedVS.cso", &blob);
    g_d3d.device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, &g_d3d.vertex_shaders[VertexShaderType_Textured]);

    D3DReadFileToBlob(L"./shaders/textured_phongVS.cso", &blob);
    g_d3d.device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, &g_d3d.vertex_shaders[VertexShaderType_TexturedPhong]);

    D3DReadFileToBlob(L"./shaders/phongVS.cso", &blob);
    g_d3d.device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, &g_d3d.vertex_shaders[VertexShaderType_UntexturedPhong]);
    g_d3d.context->VSSetShader(g_d3d.vertex_shaders[VertexShaderType_UntexturedPhong], 0, 0);

    ID3D11InputLayout* input_layout;
    D3D11_INPUT_ELEMENT_DESC input_desc[] = {
        {"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    g_d3d.device->CreateInputLayout(input_desc, ArrayCount(input_desc), blob->GetBufferPointer(), blob->GetBufferSize(), &input_layout);
    g_d3d.context->IASetInputLayout(input_layout);
}

void D3DInitConstantBuffers(RendererState* renderer) {
    RendererConstantBuffer* vso_cb = renderer->vs_obj_constant_buffer;
    D3D11_BUFFER_DESC vso_cb_desc = {
        .ByteWidth = vso_cb->max_slots * vso_cb->slot_size,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .StructureByteStride = 0
    };
    D3D11_SUBRESOURCE_DATA vso_sub_data = {};
    vso_sub_data.pSysMem = vso_cb->base_address;
    g_d3d.device->CreateBuffer(&vso_cb_desc, &vso_sub_data, &g_d3d.vs_obj_cb);

    RendererConstantBuffer* vsf_cb = renderer->vs_frame_constant_buffer;
    D3D11_BUFFER_DESC vsf_cb_desc = {
        .ByteWidth = vsf_cb->max_slots * vsf_cb->slot_size,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .StructureByteStride = 0
    };
    D3D11_SUBRESOURCE_DATA vsf_sub_data = {};
    vsf_sub_data.pSysMem = vsf_cb->base_address;
    g_d3d.device->CreateBuffer(&vsf_cb_desc, &vsf_sub_data, &g_d3d.vs_frame_cb);

    RendererConstantBuffer* psf_cb = renderer->ps_frame_constant_buffer;
    D3D11_BUFFER_DESC psf_cb_desc = {
        .ByteWidth = psf_cb->max_slots * psf_cb->slot_size,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .StructureByteStride = 0
    };
    D3D11_SUBRESOURCE_DATA psf_sub_data = {};
    psf_sub_data.pSysMem = psf_cb->base_address;
    g_d3d.device->CreateBuffer(&psf_cb_desc, &psf_sub_data, &g_d3d.ps_frame_cb);
}

void D3DInitSubresources(RendererState* renderer) {
    D3D11_BUFFER_DESC buffer_desc = {
        .ByteWidth = renderer->vertex_buffer->max_memory_size,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .StructureByteStride = sizeof(Vertex)
    };
    D3D11_SUBRESOURCE_DATA sub_rec_data = {};
    sub_rec_data.pSysMem = renderer->vertex_buffer->base_address;
    g_d3d.device->CreateBuffer(&buffer_desc, &sub_rec_data, &g_d3d.vertex_buffer);
    u32 stride = sizeof(Vertex);
    u32 offset = 0;
    g_d3d.context->IASetVertexBuffers(0, 1, &g_d3d.vertex_buffer, &stride, &offset);

    D3D11_BUFFER_DESC ibd = {
        .ByteWidth = renderer->index_buffer->max_memory_size,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .StructureByteStride = sizeof(Index)
    };
    D3D11_SUBRESOURCE_DATA isd = {};
    isd.pSysMem = renderer->index_buffer->base_address;
    g_d3d.device->CreateBuffer(&ibd, &isd, &g_d3d.index_buffer);
    g_d3d.context->IASetIndexBuffer(g_d3d.index_buffer, DXGI_FORMAT_R16_UINT, 0);

    D3DLoadShaders(renderer);

    D3DInitConstantBuffers(renderer);
}

void D3DCreateTextureResource(Texture* texture, u32 width, u32 height) {
    g_d3d.textures_2d[g_d3d.next_free_tex_slot].id = texture->id;
    D3D11_TEXTURE2D_DESC tex_desc = {
        .Width = width,
        .Height = height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0,
        },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = 0,
        .MiscFlags = 0
    };
    D3D11_SUBRESOURCE_DATA sub_data = {
        .pSysMem = texture->data,
        .SysMemPitch = width * sizeof(u32)
    };
    g_d3d.device->CreateTexture2D(&tex_desc, &sub_data, &g_d3d.textures_2d[g_d3d.next_free_tex_slot].texture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {
        .Format = tex_desc.Format,
        .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = 1
        }
    };
    g_d3d.device->CreateShaderResourceView(g_d3d.textures_2d[g_d3d.next_free_tex_slot].texture, &srv_desc, &g_d3d.textures_2d[g_d3d.next_free_tex_slot].shader_view);
    g_d3d.next_free_tex_slot++;
    DASSERT(g_d3d.next_free_tex_slot < ArrayCount(g_d3d.textures_2d));

}

void D3DCreateTextureResource(Texture* texture, u32 dimension) {
    D3DCreateTextureResource(texture, dimension, dimension);
}


void D3DBindTexture(u32 tex_id) {
    for (u32 i = 0; i < ArrayCount(g_d3d.textures_2d); i++) {
        if (g_d3d.textures_2d[i].id == tex_id) {
            g_d3d.context->PSSetShaderResources(0, 1, &g_d3d.textures_2d[i].shader_view);
            return;
        }
    }
    DERROR("D3DBindTexture: No texture with id: %u found", tex_id);
}

void D3DBindSampler() {
    g_d3d.context->PSSetSamplers(0, 1, &g_d3d.sampler);
}

void D3DSetPixelShader(PixelShaderType type) {
    g_d3d.context->PSSetShader(g_d3d.pixel_shaders[type], 0, 0);
    g_d3d.bound_pixel_shader = type;
}

void D3DRenderCommands(RendererState* renderer) {
    //g_d3d.context->RSSetState(g_d3d.solid_rs);
    //g_d3d.context->RSSetState(g_d3d.wireframe_rs);
    g_d3d.context->RSSetState(g_d3d.no_cull_rs);
    D3D11_MAPPED_SUBRESOURCE vertex_map_resource;
    g_d3d.context->Map(g_d3d.vertex_buffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &vertex_map_resource);
    MemCopy(renderer->vertex_buffer->base_address, vertex_map_resource.pData, renderer->vertex_buffer->used_memory_size);
    g_d3d.context->Unmap(g_d3d.vertex_buffer, NULL);

    D3D11_MAPPED_SUBRESOURCE index_map_resource;
    g_d3d.context->Map(g_d3d.index_buffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &index_map_resource);
    MemCopy(renderer->index_buffer->base_address, index_map_resource.pData, renderer->index_buffer->used_memory_size);
    g_d3d.context->Unmap(g_d3d.index_buffer, NULL);



    D3D11_MAPPED_SUBRESOURCE vs_obj_mr;
    u32 vs_obj_buffer_size = renderer->vs_obj_constant_buffer->slot_size * renderer->vs_obj_constant_buffer->max_slots; 
    g_d3d.context->Map(g_d3d.vs_obj_cb, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &vs_obj_mr);
    MemCopy(renderer->vs_obj_constant_buffer->base_address, vs_obj_mr.pData, vs_obj_buffer_size);
    g_d3d.context->Unmap(g_d3d.vs_obj_cb, NULL);

    D3D11_MAPPED_SUBRESOURCE vs_frame_mr;
    u32 vs_frame_buffer_size = renderer->vs_frame_constant_buffer->slot_size * renderer->vs_frame_constant_buffer->max_slots; 
    g_d3d.context->Map(g_d3d.vs_frame_cb, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &vs_frame_mr);
    MemCopy(renderer->vs_frame_constant_buffer->base_address, vs_frame_mr.pData, vs_frame_buffer_size);
    g_d3d.context->Unmap(g_d3d.vs_frame_cb, NULL);

    D3D11_MAPPED_SUBRESOURCE ps_frame_mr;
    u32 ps_frame_buffer_size = renderer->ps_frame_constant_buffer->slot_size * renderer->ps_frame_constant_buffer->max_slots; 
    g_d3d.context->Map(g_d3d.ps_frame_cb, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ps_frame_mr);
    MemCopy(renderer->ps_frame_constant_buffer->base_address, ps_frame_mr.pData, ps_frame_buffer_size);
    g_d3d.context->Unmap(g_d3d.ps_frame_cb, NULL);

    u32 num_constants = 16;
    u32 proj_view_offset = 0;
    g_d3d.context->VSSetConstantBuffers1(0, 1, &g_d3d.vs_frame_cb, NULL, NULL);
    g_d3d.context->VSSetConstantBuffers1(1, 1, &g_d3d.vs_obj_cb, NULL, NULL);
    g_d3d.context->PSSetConstantBuffers(0, 1, &g_d3d.ps_frame_cb);
    RendererCommandBuffer* command_buffer = renderer->command_buffer;
    for (RenderCommand* command = (RenderCommand*)command_buffer->base_address; (u8*)command < command_buffer->base_address + command_buffer->used_memory_size; command++) {
        u32 offset = command->vertex_constant_buffer_offset/sizeof(Vec4);

        if (command->lit) {
            if (command->texture_id == TexID_Unset) {
                g_d3d.context->VSSetShader(g_d3d.vertex_shaders[VertexShaderType_UntexturedPhong], 0, 0);
                g_d3d.context->PSSetShader(g_d3d.pixel_shaders[PixelShaderType_UntexturedPhong], 0, 0);
            } else {
                g_d3d.context->VSSetShader(g_d3d.vertex_shaders[VertexShaderType_TexturedPhong], 0, 0);
                g_d3d.context->PSSetShader(g_d3d.pixel_shaders[PixelShaderType_TexturedPhong], 0, 0);
                D3DBindTexture(command->texture_id);
            }

        } else {
                g_d3d.context->VSSetShader(g_d3d.vertex_shaders[VertexShaderType_Textured], 0, 0);
            if (command->texture_id == TexID_Unset) {
                g_d3d.context->PSSetShader(g_d3d.pixel_shaders[PixelShaderType_Untextured], 0, 0);
            } else {
                g_d3d.context->PSSetShader(g_d3d.pixel_shaders[PixelShaderType_Textured], 0, 0);
                D3DBindTexture(command->texture_id);
            }
        }


        g_d3d.context->VSSetConstantBuffers1(1, 1, &g_d3d.vs_obj_cb, &offset, &num_constants);
        g_d3d.context->IASetPrimitiveTopology(D3DGetTopology(command->topology));
        g_d3d.context->DrawIndexed(command->index_count, command->index_buffer_offset, command->vertex_buffer_offset);

    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = false;
    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    if (ImGui::Begin("Light Movement")) {
        PointLight* light = &renderer->ps_pfc.point_light;
        ImGui::SliderFloat("X", &light->position.x, -60.0f, 60.0f, "%.1f");
        ImGui::SliderFloat("Y", &light->position.y, -60.0f, 60.0f, "%.1f");
        ImGui::SliderFloat("Z", &light->position.z, -60.0f, 60.0f, "%.1f");

        ImGui::Text("Intensity/Color");
        ImGui::SliderFloat("Intensity", &light->diffuse_intensity, 0.01, 2.0f, "%.1f");
        ImGui::ColorEdit3("Diffuse Color", &light->diffuse_color.x);
        ImGui::ColorEdit3("Ambient", &light->ambient.x);
        ImGui::ColorEdit3("Material", &light->mat_color.x);

        ImGui::Text("Falloff");
        ImGui::SliderFloat("Constant", &light->att_const, 0.05f, 10.0f, "%.2f");
        ImGui::SliderFloat("Linear", &light->att_lin, 0.0001f, 4.0f, "%.4f");
        ImGui::SliderFloat("Quadratic", &light->att_quad, 0.0000001f, 10.0f, "%.2f");
    }
    ImGui::End();


    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

