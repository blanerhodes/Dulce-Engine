#pragma once

#define CAMERA_DEFAULT_YAW             -90.0f
#define CAMERA_DEFAULT_PITCH            0.0f
#define CAMERA_DEFAULT_SPEED            2.5f
#define CAMERA_DEFAULT_ROTATION_SPEED   5.0f
#define CAMERA_DEFAULT_ZOOM             45.0f
#define CAMERA_DEFAULT_WORLD_UP         {0.0f, 1.0f, 0.0f}
#define CAMERA_DEFAULT_POSITION         {0.0f, 2.0f, 5.0f} //this is in renderer coords not world coords

struct Camera {
	Vec3 position;
	Vec3 front;
	Vec3 up;
	Vec3 right;
	Vec3 world_up;
	f32 pitch;
	f32 yaw;
	f32 speed;
	f32 rotation_speed;
	f32 zoom;
};
