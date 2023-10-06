#pragma once

struct GameButtonState {
    u32 half_transition_count;
    b32 ended_down;
};

enum GameControllerKeys {
    KEY_MOVE_FORWARD,
    KEY_MOVE_DOWN,
    KEY_MOVE_LEFT,
    KEY_MOVE_RIGHT,
    KEY_ACTION_0,
    KEY_ACTION_1,
    KEY_ACTION_2,
    KEY_ACTION_3,
    KEY_LEFT_ALTERNATE,
    KEY_RIGHT_ALTERNATE,
    KEY_BACK,
    KEY_START,

    KEY_TERMINATOR
};

char* game_controller_key_strings[12] = {
    "MOVE_FORWARD   ",
    "MOVE_DOWN      ",
    "MOVE_LEFT      ",
    "MOVE_RIGHT     ",
    "ACTION_0       ",
    "ACTION_1       ",
    "ACTION_2       ",
    "ACTION_3       ",
    "LEFT_ALTERNATE ",
    "RIGHT_ALTERNATE",
    "BACK           ",
    "START          "
};

struct GameControllerInput {
    b32 is_connected;
    //b32 is_analog;    
    
    union {
        GameButtonState buttons[12];
        struct {
            GameButtonState move_forward;
            GameButtonState move_backward;
            GameButtonState move_left;
            GameButtonState move_right;
            
            GameButtonState action_0;
            GameButtonState action_1;
            GameButtonState action_2;
            GameButtonState action_3;
            
            GameButtonState left_alternate;
            GameButtonState right_alternate;

            GameButtonState menu;
            GameButtonState pause;
            GameButtonState terminator;
        };
    };
};

enum GameInputMouseButton {
    MouseButton_Left,
    MouseButton_Middle,
    MouseButton_Right,
    MouseButton_Extended0,
    MouseButton_Extended1,
    MouseButton_Count
};

struct GameInput {
    GameButtonState mouse_buttons[MouseButton_Count];
    f32 delta_time;
    i32 mouse_x;
    i32 mouse_y;
    i32 mouse_z;
    i32 mouse_x_delta;
    i32 mouse_y_delta;
    i32 mouse_wheel_delta;
    GameControllerInput controllers[2];
};
