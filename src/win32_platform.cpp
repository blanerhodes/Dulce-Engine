#include "defines.h"
#include "asserts.h"
#include "platform_services.h"
#include "core/logger.cpp"
#include "core/dmemory.h"
#include "core/dmemory.cpp"
#include "core/dstring.cpp"
#include "win32_platform.h"
#include "renderer/d3d11/dulce_dx3d11.h"
#include "game_input.h"
#include "camera.cpp"
#include "renderer/render.cpp"
#include "renderer/d3d11/dulce_dx3d11.cpp"
#include "dulce.cpp"
#include "renderer/font_atlas_maker.cpp"
#include <windows.h>
#include <ShellScalingApi.h>

static b8 g_running;
static b8 g_pause;
static i64 g_perf_count_frequency;
static WINDOWPLACEMENT g_window_position = {sizeof(g_window_position)};
Win32FrameBuffer g_back_buffer;
static HDC g_window_dc;

static GameInput input[2] = {};
static GameInput* new_input = &input[0];
static GameInput* old_input = &input[1];

static LARGE_INTEGER GetWallClock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

static f32 GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
    return (f32)(end.QuadPart - start.QuadPart) / (f32)g_perf_count_frequency;
}

static b32 PlatformFileExists(char* filename) {
    DWORD attrib = GetFileAttributesA(filename);
    b32 result = (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
    return result;
}

static DebugReadFileResult DebugPlatformReadEntireFile(ThreadContext* thread, char* filename) {
    DebugReadFileResult result = {};
    HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

    if (file_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER file_size;
        if (GetFileSizeEx(file_handle, &file_size)) {
            DASSERT(file_size.QuadPart <= 0xFFFFFFFF);
            result.contents = VirtualAlloc(0, file_size.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if (result.contents) {
                DWORD bytes_read;
                if (ReadFile(file_handle, result.contents, file_size.QuadPart, &bytes_read, 0) && file_size.QuadPart == bytes_read) {
                    //read successfully
                    result.contents_size = file_size.QuadPart;
                }
            } else {
                DebugPlatformFreeFileMemory(thread, result.contents);
                result.contents = 0;
            }
        }
        CloseHandle(file_handle);
    }
    return result;
}

static void DebugPlatformFreeFileMemory(ThreadContext* thread, void* memory) {
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

static b32 DebugPlatformWriteEntireFile(ThreadContext* thread, char* filename, u32 memory_size, void* memory) {
    b32 result = false;
    HANDLE file_handle = CreateFileA(filename, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);

    if (file_handle != INVALID_HANDLE_VALUE) {
        DWORD bytes_written;
        if (WriteFile(file_handle, memory, memory_size, &bytes_written, NULL)) {
            //success
            result = bytes_written == memory_size;
        } else {
            //log error
        }
        CloseHandle(file_handle);
    } else {
        //log error
    }
    return result;
}

static void ToggleFullscreen(HWND window) {
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx

    DWORD style = GetWindowLong(window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO monitor_info = { sizeof(monitor_info) };
        if (GetWindowPlacement(window, &g_window_position) &&
            GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor_info)) {

            SetWindowLong(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window, HWND_TOP,
                monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else {
        SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &g_window_position);
        SetWindowPos(window, 0, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

static void Win32ProcessKeyboardMessage(GameButtonState* new_state, b32 is_down) {
    if (new_state->ended_down != is_down) {
        new_state->ended_down = is_down;
        new_state->half_transition_count++;
    }
}

static void Win32ProcessPendingMessages(GameControllerInput* keyboard_controller) {
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
        switch(message.message) {
            case WM_QUIT:{
                g_running = false;
            } break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:{
                u32 vk_code = (u32)message.wParam;
                b32 was_down = ((message.lParam & (1 << 30)) != 0);
                b32 is_down = ((message.lParam & (1 << 31)) == 0);
                if (was_down != is_down) {
                    if(vk_code == 'W') {
                        Win32ProcessKeyboardMessage(&keyboard_controller->move_forward, is_down);
                    }
                    else if(vk_code == 'A')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->move_left, is_down);
                    }
                    else if(vk_code == 'S')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->move_backward, is_down);
                    }
                    else if(vk_code == 'D')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->move_right, is_down);
                    }
                    else if(vk_code == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->left_alternate, is_down);
                    }
                    else if(vk_code == 'E')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->right_alternate, is_down);
                    }
                    else if(vk_code == 'K')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->action_0, is_down);
                    }
                    else if(vk_code == 'H')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->action_1, is_down);
                    }
                    else if(vk_code == 'J')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->action_2, is_down);
                    }
                    else if(vk_code == 'L')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->action_3, is_down);
                    }
                    else if(vk_code == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->menu, is_down);
                    }
                    else if(vk_code == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->pause, is_down);
                    }
                }
                if (is_down) {
                    b32 alt_key_was_down = (message.lParam & (1 << 29));
                    if ((vk_code == VK_F4) && alt_key_was_down) {
                        g_running = false;
                    }
                    if ((vk_code == VK_RETURN) && alt_key_was_down) {
                        ToggleFullscreen(message.hwnd);
                    }
                }
            } break;
            case WM_MOUSEWHEEL: {
                i32 delta = GET_WHEEL_DELTA_WPARAM(message.wParam) / WHEEL_DELTA;
                new_input->mouse_z += delta;
                old_input->mouse_z += delta;
            } break;
            default: {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            } break;
        }
    }
}

static Win32WindowDimensions Win32GetWindowDimensions(HWND window) {
    Win32WindowDimensions result = {};
    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;
    return result;
}

static void Win32ResizeFrameBuffer(Win32FrameBuffer* buffer, u32 width, u32 height) {

    if (buffer->memory) {
        VirtualFree(buffer->memory, NULL, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height; //idk if i like this being top down
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    g_back_buffer.bytes_per_pixel = 4;
    buffer->pitch = buffer->width * g_back_buffer.bytes_per_pixel;
    u32 bitmap_size = buffer->pitch * buffer->height;
    buffer->memory = VirtualAlloc(NULL, bitmap_size, MEM_COMMIT, PAGE_READWRITE);
}

static void Win32PresentToWindow(HDC device_context, RendererState* renderer_data, f32 mouse_x, f32 mouse_y) {
    D3DRenderCommands(renderer_data);
    g_d3d.swap_chain->Present(1, 0);
}

static LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {       
    LRESULT result = 0;
    switch(message) {
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            DASSERT(!"Keyboard input came in through a non-dispatch message!");
        } break;
        case WM_CLOSE: 
        case WM_DESTROY: {
            g_running = false;
        } break;
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
            //Win32PresentToWindow(device_context, &g_back_buffer, dimensions.width, dimensions.height, 0, new_input->mouse_x, new_input->mouse_y);
            EndPaint(window, &paint);
        } break;
        default: {
            result = DefWindowProcA(window, message, w_param, l_param);
        } break;
    }
    return result;
}


int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show) {
    //SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
   //SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buffer;
   //DWORD cb_buffer;
   //u8 dummy[256] = {};
   //buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)dummy;
   //cb_buffer = 1;
   //GetLogicalProcessorInformationEx(RelationAll, buffer, &cb_buffer);
   //buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(cb_buffer);

   //GetLogicalProcessorInformationEx(RelationAll, buffer, &cb_buffer);
    Win32State win32_state = {};

    LARGE_INTEGER perf_count_frequency_result;
    QueryPerformanceFrequency(&perf_count_frequency_result);
    g_perf_count_frequency = perf_count_frequency_result.QuadPart;

    UINT desired_scheduler_ms = 1;
    b32 sleep_is_granular = timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR;

    WNDCLASSA wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = Win32MainWindowCallback;
    wc.hInstance = instance;
    wc.lpszClassName = "DulceWindowClass";
    wc.hCursor = LoadCursor(0, IDC_CROSS);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    u32 client_width = 960;
    u32 client_height = 540;
    u32 window_width = client_width;
    u32 window_height = client_height;

    u32 window_ex_style = WS_EX_APPWINDOW;
    u32 window_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    RECT border_rect = {};
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    window_width += border_rect.right - border_rect.left + 20;
    window_height += border_rect.bottom - border_rect.top + 20;

    HWND window = CreateWindowExA(window_ex_style, wc.lpszClassName, "Dulce", window_style,
                                  CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height,
                                  NULL, NULL, instance, NULL);
    if (!window) {
        MessageBoxA(0, "Failed to create window", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    g_window_dc = GetDC(window);
    //g_opengl_rc = Win32InitOpenGl(g_window_dc);

    Direct3d d3d = {};
    InitDirect3D(window, client_width, client_height);
    f32 t_sin = 0;

    Win32ResizeFrameBuffer(&g_back_buffer, client_width, client_height);

    HDC refresh_dc = GetDC(window);
    int win32_refresh_rate = GetDeviceCaps(refresh_dc, VREFRESH);
    ReleaseDC(window, refresh_dc);
    i32 monitor_refresh_hz = 60;
    if (win32_refresh_rate > 1) {
        monitor_refresh_hz = win32_refresh_rate;
    }
    f32 game_update_hz = monitor_refresh_hz / 2.0f;
    f32 target_seconds_per_frame = 1.0f / (f32)game_update_hz;

#if 0
    LPVOID base_address = 0;
#else
    LPVOID base_address = (LPVOID)TeraBytes(2);
#endif

    ThreadContext thread = {};

    GameMemory game_memory = {};
    game_memory.permanent_storage_size = MegaBytes(64);
    game_memory.transient_storage_size = GigaBytes(1);
    win32_state.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
    win32_state.game_memory_block = VirtualAlloc(base_address, win32_state.total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    game_memory.permanent_storage = win32_state.game_memory_block;
    game_memory.transient_storage = (u8*)game_memory.permanent_storage + game_memory.permanent_storage_size;
    if (!game_memory.permanent_storage || !game_memory.transient_storage) {
        DERROR("Could not initialize game memory");
        return 1;
    }

    RendererMemory renderer_memory = {};
    //NOTE: last checked the renderer wasn't even using 23Mb in total
    void* renderer_base_addr = (u8*)win32_state.game_memory_block + win32_state.total_size;
    renderer_memory.permanent_storage_size = MegaBytes(25); 
    renderer_memory.scratch_storage_size =  MegaBytes(5);
    u64 renderer_total_size = renderer_memory.permanent_storage_size + renderer_memory.scratch_storage_size;
    win32_state.total_size += renderer_total_size;
    win32_state.renderer_memory_block = VirtualAlloc(renderer_base_addr, renderer_total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    renderer_memory.permanent_storage = win32_state.renderer_memory_block;
    renderer_memory.scratch_storage = (u8*)win32_state.renderer_memory_block + renderer_memory.permanent_storage_size;
    if (!renderer_memory.permanent_storage || !renderer_memory.scratch_storage) {
        DERROR("Could not initialize renderer memory");
        return 1;
    }

    //GameInput input[2] = {};
    //GameInput* new_input = &input[0];
    //GameInput* old_input = &input[1];

 //TODO: this is temporary while i get around to making dof file gen happen as part of the build
    char* sphere_dof_path = "../../resources/assets/sphere.dof";
    char* sphere_obj_path = "../../resources/objects/sphere.obj";
    if (!PlatformFileExists(sphere_dof_path)){
        WFObjToDof(sphere_obj_path, sphere_dof_path);
    }
    char* cube_dof_path = "../../resources/assets/cube.dof";
    char* cube_obj_path = "../../resources/objects/cube.obj";
    if (!PlatformFileExists(cube_dof_path)){
        WFObjToDof(cube_obj_path, cube_dof_path);
    }

    char* suzanne_dof_path = "../../resources/assets/suzanne.dof";
    char* suzanne_obj_path = "../../resources/objects/suzanne.obj";
    if (!PlatformFileExists(suzanne_dof_path)){
        WFObjToDof(suzanne_obj_path, suzanne_dof_path);
    }

    char* font_ttf_path = "../../resources/assets/cour.ttf";
    char* font_png_path = "../../resources/assets/cour.png";
    if (!PlatformFileExists(font_png_path)) {
        BuildFontAtlas(font_ttf_path, font_png_path);
    }

    LARGE_INTEGER last_counter = GetWallClock();
    LARGE_INTEGER flip_wall_clock = GetWallClock();
    u64 last_cycle_count = __rdtsc();

    g_running = true;
    //NOTE: because we use CS_OWNDC we can grab one DC and keep reusing it since it doesnt share it
    while (g_running) {
        new_input->delta_time = target_seconds_per_frame;

        POINT mouse_pos;
        GetCursorPos(&mouse_pos);
        ScreenToClient(window, &mouse_pos);
        new_input->mouse_x = mouse_pos.x;
        new_input->mouse_y = mouse_pos.y;
        new_input->mouse_x_delta = new_input->mouse_x - old_input->mouse_x;
        new_input->mouse_y_delta = new_input->mouse_y - old_input->mouse_y;

        DWORD win_button_id[MouseButton_Count] = {
            VK_LBUTTON,
            VK_MBUTTON,
            VK_RBUTTON,
            VK_XBUTTON1,
            VK_XBUTTON2
        };
        for (u32 button_index = 0; button_index < MouseButton_Count; button_index++) {
            new_input->mouse_buttons[button_index] = old_input->mouse_buttons[button_index];
            new_input->mouse_buttons[button_index].half_transition_count = 0;
            Win32ProcessKeyboardMessage(&new_input->mouse_buttons[button_index], GetKeyState(win_button_id[button_index]) & (1 << 15));
        }

        //get input
        GameControllerInput* old_keyboard_controller = GetController(old_input, 0);
        GameControllerInput* new_keyboard_controller = GetController(new_input, 0);
        *new_keyboard_controller = {};
        new_keyboard_controller->is_connected = true;

        
        for (i32 button_index = 0; button_index < ArrayCount(new_keyboard_controller->buttons); button_index++) {
            new_keyboard_controller->buttons[button_index].ended_down = 
                old_keyboard_controller->buttons[button_index].ended_down;
        }

        //process input
        Win32ProcessPendingMessages(new_keyboard_controller);

        GameFrameBuffer frame_buffer = {};
        frame_buffer.memory = g_back_buffer.memory;
        frame_buffer.width = g_back_buffer.width;
        frame_buffer.height = g_back_buffer.height;
        frame_buffer.pitch = g_back_buffer.pitch;
        frame_buffer.bytes_per_pixel = g_back_buffer.bytes_per_pixel;

        //game update and render call
        //basically the platform layer asking the game to give it what to render (fill out the bitmap)
        //and fill the sound buffer (when i get to that) based on the input the platform layer received
        //may also pass timing info too
        RendererState* renderer_state = GameUpdateAndRender(&thread, &game_memory, &renderer_memory, &frame_buffer, input);

        LARGE_INTEGER work_counter = GetWallClock();
        f32 work_seconds_elapsed = GetSecondsElapsed(last_counter, work_counter);
        f32 seconds_elapsed_for_frame = work_seconds_elapsed;
        if (seconds_elapsed_for_frame < target_seconds_per_frame) {
            if (sleep_is_granular) {
                DWORD sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - seconds_elapsed_for_frame));
                if (sleep_ms > 0) {
                    Sleep(sleep_ms-1);
                }
            }
            f32 test_spf = GetSecondsElapsed(last_counter, GetWallClock());
            while (seconds_elapsed_for_frame < target_seconds_per_frame) {
                seconds_elapsed_for_frame = GetSecondsElapsed(last_counter, GetWallClock());
            }
        }
        else {
            //miss frame rate
        }

        LARGE_INTEGER end_counter = GetWallClock();
        f32 ms_per_frame = GetSecondsElapsed(last_counter, end_counter) * 1000.0f;
        last_counter = end_counter;

        Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
        HDC wind_dc = GetDC(window);
        Win32PresentToWindow(wind_dc, renderer_state, new_input->mouse_x, new_input->mouse_y);
        t_sin += 0.1f;
        ReleaseDC(window, wind_dc);

        //i32 mega_cycles = elapsed_cycle_count / (1000 * 1000);
        //swap old and new input
        GameInput* temp = new_input;
        new_input = old_input;
        old_input = temp;

        u64 end_cycle_count = __rdtsc();
        i64 elapsed_cycle_count = end_cycle_count - last_cycle_count;
        last_cycle_count = end_cycle_count;
        //flip_wall_clock = GetWallClock();
        //DINFO("MS per frame: %.02fms", ms_per_frame);
    }

    return 0;
}