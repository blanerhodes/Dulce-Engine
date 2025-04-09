#include "camera.h"
#include "defines.h"
#include "dmath.h"
#include "game_input.h"

namespace dx = DirectX;

DINLINE void CameraMoveForward(Camera* camera, f32 dt) {
	camera->position += camera->speed * dt * camera->front;
}

DINLINE void CameraMoveBackward(Camera* camera, f32 dt) {
	camera->position -= camera->speed * dt * camera->front;
}

DINLINE void CameraMoveLeft(Camera* camera, f32 dt) {
	camera->position -= camera->right * camera->speed * dt;
}

DINLINE void CameraMoveRight(Camera* camera, f32 dt) {
	camera->position += camera->right * camera->speed * dt;
}

DINLINE void CameraMoveUp(Camera* camera, f32 dt) {
	camera->position += camera->speed * dt * camera->up;
}

DINLINE void CameraMoveDown(Camera* camera, f32 dt) {
	camera->position -= camera->speed * dt * camera->up;
}

DINLINE void CameraUpdateVectors(Camera* camera, f32 delta_pitch, f32 delta_yaw) {
	//Vec3 new_front = {};
	////new_front.x = dcos(DegToRad(camera->yaw)) * dcos(DegToRad(camera->pitch));
	////new_front.y = dsin(DegToRad(camera->pitch));
	////new_front.z = dsin(DegToRad(camera->yaw)) * dcos(DegToRad(camera->pitch));
	//DirectX::XMVECTOR front = DirectX::XMVectorSet(camera->front.x, camera->front.y, camera->front.z, 0.0f);
	//DirectX::XMVECTOR rot_quat = DirectX::XMQuaternionRotationRollPitchYaw(DegToRad(delta_pitch), DegToRad(delta_yaw), 0.0f);
	//front = DirectX::XMVector3Rotate(front, rot_quat);
	//DirectX::XMFLOAT3 vec_front;
	//DirectX::XMStoreFloat3(&vec_front, front);
	//new_front.x = vec_front.x;
	//new_front.y = vec_front.y;
	//new_front.z = vec_front.z;
	//camera->front = UnitVector(new_front);
	//camera->right = UnitVector(CrossProd(camera->world_up, camera->front));
	//camera->up = UnitVector(CrossProd(camera->front, camera->right));
}

DINLINE void CameraAdjustYaw(Camera* camera, f32 degrees, f32 dt) {
	camera->yaw -= degrees * dt * camera->rotation_speed;
}

DINLINE void CameraAdjustPitch(Camera* camera, f32 degrees, f32 dt) {
	camera->pitch += degrees * dt * camera->rotation_speed;
	camera->pitch = DCLAMP(camera->pitch, -89.0f, 89.0f);
}

DINLINE DirectX::XMMATRIX CameraGetViewMatrix(Camera* camera) {
	return camera->view;
}

Camera MakeCamera(Vec3 pos, Vec3 world_up, f32 yaw, f32 pitch) {
	Camera result = {
		.position = pos,
		.front = {0, 0, 1},
		.world_up = world_up,
		.pitch = pitch,
		.yaw = yaw,
		.speed = CAMERA_DEFAULT_SPEED,
		.rotation_speed = CAMERA_DEFAULT_ROTATION_SPEED,
		.zoom = CAMERA_DEFAULT_ZOOM,
		.radius = 10.0f,
		.theta = 0.0f,
		.phi = 0.0f
	};
	CameraUpdateVectors(&result, 0.0f, 0.0f);
	return result;
}

DINLINE void CameraUpdate(Camera* camera, GameInput* input) {
	GameControllerInput controller = input->controllers[0];
	f32 moved_forward = 0.0f;
	f32 moved_right = 0.0f;
	if (controller.move_forward.ended_down) {
		moved_forward += camera->speed * input->delta_time;
	}
	if (controller.move_backward.ended_down) {
		moved_forward -= camera->speed * input->delta_time;
	}
	if (controller.move_left.ended_down) {
		moved_right -= camera->speed * input->delta_time;
	}
	if (controller.move_right.ended_down) {
		moved_right += camera->speed * input->delta_time;
	}
	if (input->mouse_buttons[MouseButton_Right].ended_down) {
		CameraAdjustPitch(camera, input->mouse_y_delta, input->delta_time);
		CameraAdjustYaw(camera, input->mouse_x_delta, input->delta_time);
	}

	DirectX::XMVECTOR def_forward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR def_right = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX cam_rot = DirectX::XMMatrixRotationRollPitchYaw(camera->pitch, camera->yaw, 0.0f);
	DirectX::XMVECTOR cam_target = DirectX::XMVector3TransformCoord(def_forward, cam_rot);
	cam_target = DirectX::XMVector3Normalize(cam_target);

	DirectX::XMVECTOR cam_right = DirectX::XMVector3TransformCoord(def_right, cam_rot);
	DirectX::XMVECTOR cam_forward = DirectX::XMVector3TransformCoord(def_forward, cam_rot);
	DirectX::XMVECTOR cam_up = DirectX::XMVector3Cross(cam_forward, cam_right);

	DirectX::XMVECTOR cam_pos = DirectX::XMVectorSet(camera->position.x, camera->position.y, camera->position.z, 0.0f);
	cam_pos += moved_right * cam_right;
	cam_pos += moved_forward * cam_forward;

	cam_target = cam_pos + cam_target;
	camera->view = DirectX::XMMatrixLookAtLH(cam_pos, cam_target, cam_up);

	DirectX::XMFLOAT3 pos;
	DirectX::XMStoreFloat3(&pos, cam_pos);
	camera->position = {pos.x, pos.y, pos.z};
	DirectX::XMFLOAT3 front;
	DirectX::XMStoreFloat3(&front, cam_forward);
	camera->front = {front.x, front.y, front.z};
	DirectX::XMFLOAT3 right;
	DirectX::XMStoreFloat3(&right, cam_right);
	camera->right = {right.x, right.y, right.z};
	DirectX::XMFLOAT3 up;
	DirectX::XMStoreFloat3(&up, cam_up);
	camera->up = {up.x, up.y, up.z};
}


DINLINE void CameraUpdateOrbital(Camera* camera, GameInput* input) {
	GameControllerInput controller = input->controllers[0];
	if (controller.move_forward.ended_down) {
		camera->phi += camera->rotation_speed * input->delta_time;
	}
	if (controller.move_backward.ended_down) {
		camera->phi -= camera->rotation_speed * input->delta_time;
	}
	if (controller.move_left.ended_down) {
		camera->theta -= camera->rotation_speed * input->delta_time;
	}
	if (controller.move_right.ended_down) {
		camera->theta += camera->rotation_speed * input->delta_time;
	}
	if (controller.left_alternate.ended_down) {
		camera->pitch += camera->rotation_speed * input->delta_time;
	}
	if (controller.right_alternate.ended_down) {
		camera->pitch -= camera->rotation_speed * input->delta_time;
	}

	dx::XMVECTOR pos = dx::XMVector3Transform(
							dx::XMVectorSet(0.0f, 0.0f, -camera->radius, 0.0f), 
							dx::XMMatrixRotationRollPitchYaw(DegToRad(camera->phi), -DegToRad(camera->theta), 0.0f)
						);
	dx::XMMATRIX lookat = dx::XMMatrixLookAtLH(pos, dx::XMVectorZero(), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)); 
	camera->view = lookat * dx::XMMatrixRotationRollPitchYaw(DegToRad(camera->pitch), -DegToRad(camera->yaw), 0.0f);
}