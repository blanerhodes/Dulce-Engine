#include "dulce.h"
#include "math/dmath.h"
#include "random.h"
#include "dinstrinsics.h"


static RendererState* GameUpdateAndRender(ThreadContext* context, GameMemory* game_memory, RendererMemory* renderer_memory, GameFrameBuffer* buffer, GameInput* input) {

    DASSERT((&input->controllers[0].terminator - &input->controllers[0].buttons[0]) == (ArrayCount(input->controllers[0].buttons)));
    DASSERT(sizeof(GameState) <= game_memory->permanent_storage_size);

    GameState* game_state = (GameState*)game_memory->permanent_storage;
    if (!game_memory->is_initialized) {
        //TODO: init game transient storage at some point??
        InitializeArena(&game_state->world_arena, game_memory->permanent_storage_size - sizeof(GameState), (u8*)game_memory->permanent_storage + sizeof(GameState));
        //TODO: this is temporary while i get around to making dof file gen happen as part of the build
        //char* sphere_dof_path = "../../../assets/Sphere.dof";
        //char* sphere_obj_path = "../../../objects/Sphere.obj";
        //if (!PlatformFileExists(sphere_dof_path)){
        //    WFObjToDof(sphere_obj_path, sphere_dof_path);
        //}
        //StringCopy((u8*)sphere_dof_path, (u8*)renderer_data->assets[AssetID_Sphere].filename);

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
        u32 vertex_constant_buffer_size = MAX_UNIFORM_BUFFER_SLOTS * UNIFORM_BUFFER_SLOT_SIZE + sizeof(RendererConstantBuffer);
        u32 texture_buffer_size = sizeof(RendererTextureBuffer) + MAX_TEXTURE_SLOTS * TEXTURE_DIM_512 * TEXTURE_DIM_512 * sizeof(u32); //this is about 4MB
        u32 asset

        RendererInitCommandBuffer(renderer_state, command_buffer_size);
        RendererInitVertexBuffer(renderer_state, vertex_buffer_size);
        RendererInitIndexBuffer(renderer_state, index_buffer_size);
        RendererInitConstantBuffer(renderer_state);
        RendererInitTextureBuffer(renderer_state, TEXTURE_DIM_512);
        D3DInitSubresources(renderer_state);

        renderer_memory->is_initialized = true;
    }

    RendererPerFrameReset(game_state, renderer_state, input);

    f32 adjusted_mouse_x = (f32)input->mouse_x / (buffer->width * 0.5f) - 1.0f;
    f32 adjusted_mouse_y = (f32)input->mouse_y / (buffer->height * 0.5f) - 3.0f;
    f32 mouse_scroll = (f32)input->mouse_z;
    static Vec3 point_light_pos = {1.0f, 1.0f, 9.0f};
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
    PointLight point_light = {
        //.position = {game_state->t_sin, 0, game_state->t_sin},
        //.position = {adjusted_mouse_x, mouse_scroll, -adjusted_mouse_y},
        .position = point_light_pos,
        .color = COLOR_WHITE,
        .intensity = 1.0f
    };
    RendererPushPointLight(renderer_state, point_light);

    BasicMesh light_plane = {
        .position = point_light.position,
        .scale = {0.5f, 0.5f, 1.0f},
        .rotation_angles = {DegToRad(90.0f), 0, 0},
        .color = COLOR_WHITEA,
        .texture_id = TexID_NoTexture
    };
    RendererPushPlane(renderer_state, light_plane);

    BasicMesh cube = {
        .position = {0.0f, 2.0f, 2.0f},
        .scale = {1.0f, 1.0f, 1.0f},
        .rotation_angles = {0, game_state->t_sin*0.2f, game_state->t_sin*0.2f},
        //.rotation_angles = {},
        .color = COLOR_REDA,
        .texture_id = TexID_NoTexture
    };
    RendererPushCube(renderer_state, cube);

    BasicMesh pyramid = {
        .position = {2.0f, 2.0f, 2.0f},
        .scale = {0.5f, 1.0f, 4.0f},
        .rotation_angles = {0, game_state->t_sin*0.2f, game_state->t_sin*0.2f},
        //.rotation_angles = {0, 0, 0},
        .color = COLOR_BLUEA,
        .texture_id = TexID_NoTexture
    };
    RendererPushPyramid(renderer_state, pyramid);

    BasicMesh cone = {
        .position = {-2.0f, 2.0f, 2.0f},
        .scale = {0.5f, 1.0f, 50.0f},
        .rotation_angles = {0, game_state->t_sin*0.2f, game_state->t_sin*0.2f},
        //.rotation_angles = {0, 0, 0},
        .color = COLOR_GREENA,
        .texture_id = TexID_NoTexture
    };
    RendererPushCone(renderer_state, cone);

    BasicMesh cylinder = {
        .position = {-5.0f, 2.0f, 2.0f},
        .scale = {0.5f, 1.0f, 50.0f},
        .rotation_angles = {0, game_state->t_sin*0.2f, game_state->t_sin*0.2f},
        //.rotation_angles = {0, 0, 0},
        .color = COLOR_MAGENTAA,
        .texture_id = TexID_NoTexture
    };
    RendererPushCylinder(renderer_state, cylinder);

    BasicMesh plane = {
        .position = {0.0f, 0.0f, 0.0f},
        .scale = {20.0f, 20.0f, 1.0f},
        .rotation_angles = {0, 0, 0},
        .color = COLOR_CYANA,
        .texture_id = TexID_NoTexture
    };
    RendererPushPlane(renderer_state, plane);

    BasicMesh sky_plane = {
        .position = {0, 30.0f, 0.0f},
        .scale = {50.0f, 50.0f, 1.0f},
        .rotation_angles = {DegToRad(90.0f), 0, 0},
        .color = COLOR_WHITEA,
        .texture_id = TexID_Sky
    };
    RendererPushPlane(renderer_state, sky_plane);

    //static u32 frame_count = 0;
    //if (frame_count % 10 == 0) {
    //    //DINFO("%.4f\n", game_state->t_sin);
    //    DINFO("t_sin: %.4f --- sin(): %.4f\n", game_state->t_sin, dsin(game_state->t_sin));
    //}
    game_state->t_sin += 0.05f;
    
    
    return renderer_state;
}

