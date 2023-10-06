#include "dulce_dx3d11.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3d11sdklayers.h>

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


//static D3DState renderer_state = {};

typedef HRESULT DXGIGetDebugInterface_ (REFIID, void**);
static DXGIGetDebugInterface_* DxgiGetDebugInterface;

static void InitDirect3D(HWND hwnd, u32 view_width, u32 view_height) {
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
    swapchain_desc.Width = view_width;
    swapchain_desc.Height = view_height;
    swapchain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapchain_desc.Stereo = FALSE;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1; //2 for d7s stuff
    swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
    //If anyone is using FLIP_DISCARD / FLIP_SEQUENTIAL don't remove SetRenderTargets call from DrawTriangle.
    //Add the depthstencilview pointer instead of nullptr there. As FLIP mode removes binds every frame/flip.
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; //use this later DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchain_desc.Flags = 0;

    g_d3d.dxgi_factory->CreateSwapChainForHwnd(g_d3d.device, hwnd, &swapchain_desc, 0, 0, &g_d3d.swap_chain);
    g_d3d.swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&g_d3d.frame_buffer);
    g_d3d.device->CreateRenderTargetView(g_d3d.frame_buffer, 0, &g_d3d.frame_buffer_view);

    D3D11_TEXTURE2D_DESC depth_buffer_desc = {};
    g_d3d.frame_buffer->GetDesc(&depth_buffer_desc); //copy from framebuffer properties
    depth_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    g_d3d.device->CreateTexture2D(&depth_buffer_desc, 0, &g_d3d.depth_buffer);
    g_d3d.device->CreateDepthStencilView(g_d3d.depth_buffer, 0, &g_d3d.depth_buffer_view);

    D3D11_RASTERIZER_DESC1 rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = D3D11_CULL_NONE; //this doesnt seem like its working
    g_d3d.device->CreateRasterizerState1(&rasterizer_desc, &g_d3d.rasterizer_state);

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    g_d3d.device->CreateSamplerState(&sampler_desc, &g_d3d.sampler);

    D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};
    depth_stencil_desc.DepthEnable = TRUE;
    depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
    g_d3d.device->CreateDepthStencilState(&depth_stencil_desc, &g_d3d.depth_stencil_state);

    g_d3d.view_width = view_width;
    g_d3d.view_height = view_height;
    g_d3d.aspect_ratio = (f32)view_width / (f32)view_height;

    g_d3d.context->OMSetRenderTargets(1, &g_d3d.frame_buffer_view, g_d3d.depth_buffer_view);
    g_d3d.context->OMSetDepthStencilState(g_d3d.depth_stencil_state, 0);
    g_d3d.context->OMSetBlendState(0, 0, 0xFFFFFFFF); //NOTE: default blend mode

    g_d3d.viewport = {0.0f, 0.0f, (f32)depth_buffer_desc.Width, (f32)depth_buffer_desc.Height, 0.0f, 1.0f};
    g_d3d.context->RSSetViewports(1, &g_d3d.viewport);

    g_d3d.next_free_tex_slot = 0;
}

static void D3DClearBuffer(f32 red, f32 green, f32 blue) {
    Vec4 color = {red, green, blue, 1.0f};
    g_d3d.context->ClearRenderTargetView(g_d3d.frame_buffer_view, (f32*)&color);
    g_d3d.context->ClearDepthStencilView(g_d3d.depth_buffer_view, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

static D3D11_PRIMITIVE_TOPOLOGY D3DGetTopology(RenderTopology topology) {
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


static void D3DInitSubresources(RendererState* renderer) {
    D3D11_BUFFER_DESC buffer_desc = {
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .Usage = D3D11_USAGE_DYNAMIC,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .ByteWidth = renderer->vertex_buffer->max_memory_size,
        .StructureByteStride = sizeof(Vertex)
    };
    D3D11_SUBRESOURCE_DATA sub_rec_data = {};
    sub_rec_data.pSysMem = renderer->vertex_buffer->base_address;
    g_d3d.device->CreateBuffer(&buffer_desc, &sub_rec_data, &g_d3d.vertex_buffer);
    u32 stride = sizeof(Vertex);
    u32 offset = 0;
    g_d3d.context->IASetVertexBuffers(0, 1, &g_d3d.vertex_buffer, &stride, &offset);

    D3D11_BUFFER_DESC ibd = {
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
        .Usage = D3D11_USAGE_DYNAMIC,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .ByteWidth = renderer->index_buffer->max_memory_size,
        .StructureByteStride = sizeof(Index)
    };
    D3D11_SUBRESOURCE_DATA isd = {};
    isd.pSysMem = renderer->index_buffer->base_address;
    g_d3d.device->CreateBuffer(&ibd, &isd, &g_d3d.index_buffer);
    g_d3d.context->IASetIndexBuffer(g_d3d.index_buffer, DXGI_FORMAT_R16_UINT, 0);

    //TODO: probably going to need an array of shaders to choose from and those D3D specific shaders will be wrapped in a 
    //      renderer agnostic shader struct that will pick the right thing based on the renderer. Same with input layout
    ID3DBlob* tex_pixel_blob;
    D3DReadFileToBlob(L"textured_pixel.cso", &tex_pixel_blob);
    g_d3d.device->CreatePixelShader(tex_pixel_blob->GetBufferPointer(),
                                    tex_pixel_blob->GetBufferSize(),
                                    0,
                                    &g_d3d.pixel_shaders[PixelShaderType_Textured]);

    ID3DBlob* untex_pixel_blob;
    ID3D11PixelShader* untex_pixel_shader = g_d3d.pixel_shaders[PixelShaderType_Untextured];
    D3DReadFileToBlob(L"untextured_pixel.cso", &untex_pixel_blob);
    g_d3d.device->CreatePixelShader(untex_pixel_blob->GetBufferPointer(),
                                    untex_pixel_blob->GetBufferSize(),
                                    0,
                                    &g_d3d.pixel_shaders[PixelShaderType_Untextured]);

    ID3DBlob* vert_blob;
    ID3D11VertexShader* vertex_shader = g_d3d.vertex_shader;
    D3DReadFileToBlob(L"textured_vertex.cso", &vert_blob);
    g_d3d.device->CreateVertexShader(vert_blob->GetBufferPointer(), vert_blob->GetBufferSize(), 0, &vertex_shader);
    g_d3d.context->VSSetShader(vertex_shader, 0, 0);

    RendererConstantBuffer* const_buffer = renderer->vertex_constant_buffer;
    D3D11_BUFFER_DESC proj_view_buffer_desc = {
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .Usage = D3D11_USAGE_DYNAMIC,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .ByteWidth = const_buffer->slot_size * const_buffer->num_per_frame_slots,
        .StructureByteStride = 0
    };
    D3D11_SUBRESOURCE_DATA proj_view_sub_data = {};
    proj_view_sub_data.pSysMem = const_buffer->base_address;
    g_d3d.device->CreateBuffer(&proj_view_buffer_desc, &proj_view_sub_data, &g_d3d.proj_view_buffer);

    D3D11_BUFFER_DESC const_buffer_desc = {
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .Usage = D3D11_USAGE_DYNAMIC,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .ByteWidth = const_buffer->max_slots * const_buffer->slot_size - proj_view_buffer_desc.ByteWidth,
        .StructureByteStride = 0
    };
    D3D11_SUBRESOURCE_DATA const_sub_data = {};
    const_sub_data.pSysMem = const_buffer->base_address + (const_buffer->slot_size * const_buffer->num_per_frame_slots);
    g_d3d.device->CreateBuffer(&const_buffer_desc, &const_sub_data, &g_d3d.constant_buffer);

    ID3D11InputLayout* input_layout;
    D3D11_INPUT_ELEMENT_DESC input_desc[] = {
        {"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEX", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    g_d3d.device->CreateInputLayout(input_desc, ArrayCount(input_desc), vert_blob->GetBufferPointer(), vert_blob->GetBufferSize(), &input_layout);
    g_d3d.context->IASetInputLayout(input_layout);
}

static void D3DCreateTextureResource(Texture* texture, u32 dimension) {
    g_d3d.textures_2d[g_d3d.next_free_tex_slot].id = texture->id;
    D3D11_TEXTURE2D_DESC tex_desc = {
        .Width = dimension,
        .Height = dimension,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
        .SampleDesc.Count = 1,
        .SampleDesc.Quality = 0,
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = 0,
        .MiscFlags = 0
    };
    D3D11_SUBRESOURCE_DATA sub_data = {
        .pSysMem = texture->data,
        .SysMemPitch = dimension*4 //sizeof(u32)
    };
    g_d3d.device->CreateTexture2D(&tex_desc, &sub_data, &g_d3d.textures_2d[g_d3d.next_free_tex_slot].texture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {
        .Format = tex_desc.Format,
        .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
        .Texture2D.MostDetailedMip = 0,
        .Texture2D.MipLevels = 1
    };
    g_d3d.device->CreateShaderResourceView(g_d3d.textures_2d[g_d3d.next_free_tex_slot].texture, &srv_desc, &g_d3d.textures_2d[g_d3d.next_free_tex_slot].shader_view);
    g_d3d.next_free_tex_slot++;
    DASSERT(g_d3d.next_free_tex_slot < ArrayCount(g_d3d.textures_2d));
}

static void D3DBindTexture(u32 tex_id) {
    for (u32 i = 0; i < ArrayCount(g_d3d.textures_2d); i++) {
        if (g_d3d.textures_2d[i].id == tex_id) {
            g_d3d.context->PSSetShaderResources(0, 1, &g_d3d.textures_2d[i].shader_view);
            return;
        }
    }
    DERROR("D3DBindTexture: No texture with id: %u found", tex_id);
}

static void D3DBindSampler() {
    g_d3d.context->PSSetSamplers(0, 1, &g_d3d.sampler);
}

static void D3DSetPixelShader(PixelShaderType type) {
    g_d3d.context->PSSetShader(g_d3d.pixel_shaders[type], 0, 0);
    g_d3d.bound_pixel_shader = type;
}

static void D3DRenderCommands(RendererState* renderer) {
    D3D11_MAPPED_SUBRESOURCE vertex_map_resource;
    g_d3d.context->Map(g_d3d.vertex_buffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &vertex_map_resource);
    MemCopy(renderer->vertex_buffer->base_address, vertex_map_resource.pData, renderer->vertex_buffer->used_memory_size);
    g_d3d.context->Unmap(g_d3d.vertex_buffer, NULL);

    D3D11_MAPPED_SUBRESOURCE index_map_resource;
    g_d3d.context->Map(g_d3d.index_buffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &index_map_resource);
    MemCopy(renderer->index_buffer->base_address, index_map_resource.pData, renderer->index_buffer->used_memory_size);
    g_d3d.context->Unmap(g_d3d.index_buffer, NULL);

    u32 frame_section_size = renderer->vertex_constant_buffer->num_per_frame_slots * renderer->vertex_constant_buffer->slot_size;
    D3D11_MAPPED_SUBRESOURCE proj_view_map_resource;
    g_d3d.context->Map(g_d3d.proj_view_buffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &proj_view_map_resource);
    MemCopy(renderer->vertex_constant_buffer->base_address, proj_view_map_resource.pData, frame_section_size);
    g_d3d.context->Unmap(g_d3d.proj_view_buffer, NULL);

    u32 object_section_size = renderer->vertex_constant_buffer->num_per_object_slots * renderer->vertex_constant_buffer->slot_size;
    D3D11_MAPPED_SUBRESOURCE constant_map_resource;
    g_d3d.context->Map(g_d3d.constant_buffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &constant_map_resource);
    MemCopy(renderer->vertex_constant_buffer->base_address + frame_section_size, constant_map_resource.pData, object_section_size);
    g_d3d.context->Unmap(g_d3d.constant_buffer, NULL);

    u32 num_constants = 16;
    u32 proj_view_offset = 0;
    g_d3d.context->VSSetConstantBuffers1(0, 1, &g_d3d.proj_view_buffer, &proj_view_offset, &num_constants);
    RendererCommandBuffer* command_buffer = renderer->command_buffer;
    for (RenderCommand* command = (RenderCommand*)command_buffer->base_address; (u8*)command < command_buffer->base_address + command_buffer->used_memory_size; command++) {
        u32 offset = command->vertex_constant_buffer_offset/sizeof(Vec4);

        if (command->texture_id == TexID_NoTexture) {
            if (g_d3d.bound_pixel_shader != PixelShaderType_Untextured) {
                D3DSetPixelShader(PixelShaderType_Untextured);
            }
        }
        else {
            D3DBindTexture(command->texture_id);
            if (g_d3d.bound_pixel_shader != PixelShaderType_Textured) {
                D3DSetPixelShader(PixelShaderType_Textured);
            }
        }

        g_d3d.context->VSSetConstantBuffers1(1, 1, &g_d3d.constant_buffer, &offset, &num_constants);
        g_d3d.context->IASetPrimitiveTopology(D3DGetTopology(command->topology));
        g_d3d.context->DrawIndexed(command->index_count, command->index_buffer_offset, command->vertex_buffer_offset);
    }
}

