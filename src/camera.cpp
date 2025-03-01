#include "camera.h"



DINLINE void CameraMoveForward(Camera* camera, f32 dt) {
	camera->position += camera->speed * dt * camera->front;
}

DINLINE void CameraMoveBackward(Camera* camera, f32 dt) {
	camera->position -= camera->speed * dt * camera->front;
}

DINLINE void CameraMoveLeft(Camera* camera, f32 dt) {
	camera->position -= UnitVector(CrossProd(camera->front, camera->up)) * camera->speed * dt;
}

DINLINE void CameraMoveRight(Camera* camera, f32 dt) {
	camera->position += UnitVector(CrossProd(camera->front, camera->up)) * camera->speed * dt;
}

DINLINE void CameraMoveUp(Camera* camera, f32 dt) {
	camera->position += camera->speed * dt * camera->up;
}

DINLINE void CameraMoveDown(Camera* camera, f32 dt) {
	camera->position -= camera->speed * dt * camera->up;
}

DINLINE void CameraUpdateVectors(Camera* camera) {
	Vec3 new_front = {};
	new_front.x = dcos(DegToRad(camera->yaw)) * dcos(DegToRad(camera->pitch));
	new_front.y = dsin(DegToRad(camera->pitch));
	new_front.z = dsin(DegToRad(camera->yaw)) * dcos(DegToRad(camera->pitch));
	camera->front = UnitVector(new_front);
	camera->right = UnitVector(CrossProd(camera->front, camera->world_up));
	camera->up = UnitVector(CrossProd(camera->right, camera->front));
}

DINLINE void CameraAdjustYaw(Camera* camera, f32 degrees, f32 dt) {
	camera->yaw += degrees * dt * camera->rotation_speed;
	CameraUpdateVectors(camera);
}

DINLINE void CameraAdjustPitch(Camera* camera, f32 degrees, f32 dt) {
	degrees *= -1;
	camera->pitch += degrees * dt * camera->rotation_speed;
	camera->pitch = DCLAMP(camera->pitch, -89.0f, 89.0f);
	CameraUpdateVectors(camera);
}

DINLINE Mat4 CameraGetViewMatrix(Camera* camera) {
	//TODO: fix Mat4LookAt() to be column vector focused, it's row vector right now
	return Mat4Transpose(Mat4LookAt(camera->position, camera->position+camera->front, camera->up));
}

Camera MakeCamera(Vec3 pos, Vec3 world_up, f32 yaw, f32 pitch) {
	Camera result = {
		.position = pos,
		.front = {0, 0, -1},
		.world_up = world_up,
		.pitch = pitch,
		.yaw = yaw,
		.speed = CAMERA_DEFAULT_SPEED,
		.rotation_speed = CAMERA_DEFAULT_ROTATION_SPEED,
		.zoom = CAMERA_DEFAULT_ZOOM
	};
	CameraUpdateVectors(&result);
	return result;
}

DINLINE void CameraUpdate(Camera* camera, GameInput* input) {
	GameControllerInput controller = input->controllers[0];
	if (controller.move_forward.ended_down) {
		CameraMoveForward(camera, input->delta_time);
	}
	if (controller.move_backward.ended_down) {
		CameraMoveBackward(camera, input->delta_time);
	}
	if (controller.move_left.ended_down) {
		CameraMoveLeft(camera, input->delta_time);
	}
	if (controller.move_right.ended_down) {
		CameraMoveRight(camera, input->delta_time);
	}
	if (input->mouse_buttons[MouseButton_Right].ended_down) {
		CameraAdjustPitch(camera, input->mouse_y_delta, input->delta_time);
		CameraAdjustYaw(camera, input->mouse_x_delta, input->delta_time);
	}
}
