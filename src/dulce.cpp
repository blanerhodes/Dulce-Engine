#pragma once
#include "dulce.h"
#include "dmath.h"
#include "dxmath.h"
#include "dinstrinsics.h"
#include "game_input.h"
#include "renderer/render.h"
#include "asserts.h"
#include "renderer/d3d11/dulce_dx3d11.h"
#include "renderer/renderer_shape_gen.h"
#include <DirectXMath.h>

static char* GameControllerKeyIndexToString(GameControllerKeys key) {
    char* result = game_controller_key_strings[key];
    return result;
}

inline GameControllerInput* GetController(GameInput* input, u32 controller_index) {
    DASSERT(controller_index < ArrayCount(input->controllers));
    
    GameControllerInput* result = &input->controllers[controller_index];
    return result;
}

static RendererState* GameUpdateAndRender(ThreadContext* context, GameMemory* game_memory, RendererMemory* renderer_memory, GameFrameBuffer* buffer, GameInput* input) {

    DASSERT((&input->controllers[0].terminator - &input->controllers[0].buttons[0]) == (ArrayCount(input->controllers[0].buttons)));
    DASSERT(sizeof(GameState) <= game_memory->permanent_storage_size);

    GameState* game_state = (GameState*)game_memory->permanent_storage;
    if (!game_memory->is_initialized) {
        //TODO: init game transient storage at some point??
        InitializeArena(&game_state->world_arena, game_memory->permanent_storage_size - sizeof(GameState), (u8*)game_memory->permanent_storage + sizeof(GameState));
       
        game_state->t_sin = 0.0f;
        game_state->camera = MakeCamera(CAMERA_DEFAULT_POSITION, CAMERA_DEFAULT_WORLD_UP, CAMERA_DEFAULT_YAW, CAMERA_DEFAULT_PITCH);
        game_memory->is_initialized = true;
    }

    RendererState* renderer_state = (RendererState*)renderer_memory->permanent_storage;
    if (!renderer_memory->is_initialized) {
        InitializeArena(&renderer_state->permanent_storage, renderer_memory->permanent_storage_size - sizeof(RendererState), (u8*)renderer_memory->permanent_storage + sizeof(RendererState));
        InitializeArena(&renderer_state->scratch_storage, renderer_memory->scratch_storage_size, (u8*)renderer_memory->scratch_storage);

        u32 command_buffer_size = MegaBytes(2) + sizeof(RendererCommandBuffer);
        u32 vertex_buffer_size = MegaBytes(2) + sizeof(RendererVertexBuffer);
        u32 index_buffer_size = MegaBytes(2) + sizeof(RendererIndexBuffer); 
        u32 texture_buffer_size = sizeof(RendererTextureBuffer) + MAX_TEXTURE_SLOTS * TEXTURE_DIM_512 * TEXTURE_DIM_512 * sizeof(u32); //this is about 4MB
        u32 asset_buffer_size = KiloBytes(1);

        RendererInitCommandBuffer(renderer_state, command_buffer_size);
        RendererInitVertexBuffer(renderer_state, vertex_buffer_size);
        RendererInitIndexBuffer(renderer_state, index_buffer_size);
        RendererInitConstantBuffers(renderer_state, 32, sizeof(PerObjectConstants), 1, sizeof(VSPerFrameConstants), 1, sizeof(PSPerFrameConstants));
        RendererInitTextureBuffer(renderer_state, TEXTURE_DIM_512);
        RendererInitAssets(renderer_state, 16);
        D3DInitSubresources(renderer_state);
        renderer_state->projection = DirectX::XMMatrixPerspectiveFovLH(45.0f, 1.0f / g_d3d.aspect_ratio, 0.1f, 1000.0f);

        renderer_state->ps_pfc.point_light = {
            .position = {0.0f, 0.0f, 0.0f},
            .mat_color = {0.7f, 0.7f, 0.9f},
            .ambient = {0.05f, 0.05f, 0.05f},
            .diffuse_color = {1.0f, 1.0f, 1.0f},
            .diffuse_intensity = 1.0f,
            .att_const = 1.0f,
            .att_lin = 0.045f,
            .att_quad = 0.0075f
        };

        renderer_memory->is_initialized = true;
    }

    RendererPerFrameReset(game_state, renderer_state, input);

    f32 adjusted_mouse_x = (f32)input->mouse_x / (buffer->width * 0.5f) - 1.0f;
    f32 adjusted_mouse_y = -(f32)input->mouse_y / (buffer->height * 0.5f) + 1.0f;
    f32 mouse_scroll = (f32)input->mouse_z;
    static Vec3 point_light_pos = {0.0f, 0.0f, 0.0f};
    if (input->controllers[0].action_0.ended_down) {
        point_light_pos.z += 0.1;
    }
    if (input->controllers[0].action_1.ended_down) {
        point_light_pos.x -= 0.1;
    }
    if (input->controllers[0].action_2.ended_down) {
        point_light_pos.z -= 0.1;
    }
    if (input->controllers[0].action_3.ended_down) {
        point_light_pos.x += 0.1;
    }
    if (input->controllers[0].left_alternate.ended_down) {
        point_light_pos.y -= 0.1;
    }
    if (input->controllers[0].right_alternate.ended_down) {
        point_light_pos.y += 0.1;
    }


    game_state->t_sin += input->delta_time;
    RendererPushClear(COLOR_GREY);

    BasicMesh cube = {
        .position = {2.0f, 0.0f, 3.0f},
        .scale = {0.5f, 0.5f, 0.5f},
        .rotation_angles = {0, 0.0f, 45.0f},
        .color = COLOR_REDA,
        .texture_id = TexID_Unset,
        .lit = true
    };
    RendererPushCube(renderer_state, cube);

    BasicMesh cube1 = {
        .position = {-1.0f, 1.0f, 5.0f},
        .scale = {1.0f, 1.0f, 1.0f},
        .rotation_angles = {game_state->t_sin, game_state->t_sin, 0},
        .color = COLOR_REDA,
        .texture_id = TexID_Unset, 
        .lit = true
    };
    RendererPushCube(renderer_state, cube1);

    BasicMesh light_cube = {
        .position = {renderer_state->ps_pfc.point_light.position},
        .scale = {0.25f, 0.25f, 0.25f},
        .color = COLOR_REDA,
        .texture_id = TexID_Unset,
        .lit = true
    };
    RendererPushCube(renderer_state, light_cube);
    RendererPushPointLight(renderer_state);

    BasicMesh pyramid = {
        .position = {2.0f, 2.0f, 2.0f},
        .scale = {0.5f, 1.0f, 4.0f},
        .rotation_angles = {0, game_state->t_sin, game_state->t_sin},
        //.rotation_angles = {0, 0, 0},
        .color = COLOR_BLUEA,
        .texture_id = TexID_Unset,
        .lit = true
    };
    RendererPushPyramid(renderer_state, pyramid);

    BasicMesh cone = {
        .position = {-2.0f, 2.0f, 2.0f},
        .scale = {0.5f, 1.0f, 50.0f},
        .rotation_angles = {0, game_state->t_sin* 20, game_state->t_sin * 20},
        //.rotation_angles = {0, 0, 0},
        .color = COLOR_GREENA,
        .texture_id = TexID_Unset,
        .lit = true
    };
    RendererPushCone(renderer_state, cone);

    BasicMesh cylinder = {
        .position = {-5.0f, 2.0f, 2.0f},
        .scale = {0.5f, 1.0f, 50.0f},
        .rotation_angles = {0, game_state->t_sin, game_state->t_sin},
        //.rotation_angles = {0, 0, 0},
        .color = COLOR_MAGENTAA,
        .texture_id = TexID_Unset,
        .lit = true
    };
    RendererPushCylinder(renderer_state, cylinder);

    BasicMesh plane = {
        .position = {0.0f, -3.0f, 0.0f},
        .scale = {20.0f, 20.0f, 1.0f},
        .rotation_angles = {90, 0, 0},
        .color = COLOR_CYANA,
        .texture_id = TexID_Pic,
        .lit = true
    };
    RendererPushPlane(renderer_state, plane);


    BasicMesh suzanne = {
        .position = {-3.0f, 2.0f, 4.0f},
        .scale = {1, 1, 1},
        //.rotation_angles = {0, game_state->t_sin * 20.0f, game_state->t_sin * 20.0f},
        .color = COLOR_WHITEA,
        .texture_id = TexID_Unset,
        .asset_id = {.name = "suzanna"},
        .lit = true
    };
    RendererPushAsset(renderer_state, suzanne);

    BasicMesh sphere = {
        .position = {4.0f, 2.0f, 4.0f},
        .scale = {1, 1, 1},
        .rotation_angles = {0, game_state->t_sin * 20.0f, game_state->t_sin * 20.0f},
        .color = COLOR_WHITEA,
        .texture_id = TexID_Unset,
        .asset_id = {.name = "sphere"},
        .lit = true
    };
    RendererPushAsset(renderer_state, sphere);


    BasicMesh sky_front = {
        .position = {0.0f, 0.0f, 50.0f},
        .scale = {150, 100, 1},
        .texture_id = TexID_Sky
    };
    RendererPushPlane(renderer_state, sky_front);

    BasicMesh sky_left = {
        .position = {100.0f, 0.0f, 0.0f},
        .scale = {200, 150, 1},
        .rotation_angles = {0.0f, -90.0f, 0.0f},
        .texture_id = TexID_Sky
    };
    RendererPushPlane(renderer_state, sky_left);

    BasicMesh sky_right = {
        .position = {-100.0f, 0.0f, 0.0f},
        .scale = {200, 150, 1},
        .rotation_angles = {0.0f, 90.0f, 0.0f},
        .texture_id = TexID_Sky
    };
    RendererPushPlane(renderer_state, sky_right);

    BasicMesh sky_back = {
        .position = {0.0f, 0.0f, -50.0f},
        .scale = {200, 150, 1},
        .rotation_angles = {0.0f, 180.0f, 0.0f},
        .texture_id = TexID_Sky
    };
    RendererPushPlane(renderer_state, sky_back);

    /*


    //BasicMesh grid = {
    //    .position = {0.0f, 0.0f, 0.0f},
    //    .scale = {1.0f, 1.0f, 1.0f},
    //    .rotation_angles = {0, 0, 0},
    //    .color = COLOR_CYANA,
    //    .texture_id = TexID_NoTexture
    //};
    //RendererPushGrid(renderer_state, 6, 6, 3, 3, grid);

    */
    
    return renderer_state;
}

