#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/Camera.h"
#include "../Common/Input.h"
using namespace Math;
struct CameraMove
{
private:
	Camera* cam;
	float2 euler = {0,0};
	Vector3 position = {0,0,0};
	const float walkSpeed = 4;
	const float accelaration = 4;
	float speed = 0;
public:
	CameraMove(Camera* cam) : 
		cam(cam)
	{
		
	}

	static float3 GetMovementVector()
	{
		float3 a = { 0,0,0 };
		
		if (Input::IsKeyPressed(KeyCode::W))
		{
			a.z = 1;
		}
		if (Input::IsKeyPressed(KeyCode::S))
		{
			a.z -= 1;
		}
		if (Input::IsKeyPressed(KeyCode::A))
		{
			a.x = -1;
		}
		if (Input::IsKeyPressed(KeyCode::D))
		{
			a.x += 1;
		}
		if (Input::IsKeyPressed(KeyCode::Q)) {
			a.y = -1;
		}
		if (Input::IsKeyPressed(KeyCode::E))
		{
			a.y += 1;
		}
		return a;
	}

	static float2 GetMouseRotation()
	{
	//	if (Input::IsKeyPressed(KeyCode::G))
		//{

			int2 mouse = Input::MouseMovement();
			// Make each pixel correspond to a quarter of a degree.
			float dx = XMConvertToRadians(0.2f * static_cast<float>(mouse.x));
			float dy = XMConvertToRadians(0.2f * static_cast<float>(mouse.y));
			return { dx, dy };
	}

	void Run(float deltaTime)
	{
		position = cam->GetPosition();
		Vector3 movementVector = GetMovementVector();
		speed = dot(abs(movementVector), Vector3(1)) < 0.01 ? walkSpeed : speed + accelaration * deltaTime;
		
		movementVector *= deltaTime * (Input::IsKeyPressed(KeyCode::Shift) ? (speed * 2) : speed);
		float2 mouseRotation = GetMouseRotation();
		euler.x -= mouseRotation.x;
		euler.y += mouseRotation.y;
		euler.y = MathHelper::Clamp(euler.y, 0.1f, MathHelper::Pi - 0.1f);
		Vector3 eyePos(
			sinf(euler.y) * cosf(euler.x),
			cosf(euler.y),
			sinf(euler.y) * sinf(euler.x));
		cam->LookAt({ 0,0,0 }, (float3)eyePos, { 0,1,0 });
		position += cam->GetRight() *movementVector.GetX() + cam->GetUp() * movementVector.GetY() + cam->GetLook() * movementVector.GetZ();
		cam->SetPosition(position);
	}
};