#include "pch.h"
#include "TestCameraScript.h"
#include "Transform.h"
#include "Engine.h"

TestCameraScript::TestCameraScript()
{
}

TestCameraScript::~TestCameraScript()
{
}

void TestCameraScript::LateUpdate()
{
	Vec3 pos = GetTransform()->GetLocalPosition();

	if (GEngine->GetButton(KEY_TYPE::W))
		pos += GetTransform()->GetLook() * _speed * GEngine->GetDeltaTime();

	if (GEngine->GetButton(KEY_TYPE::S))
		pos -= GetTransform()->GetLook() * _speed * GEngine->GetDeltaTime();

	if (GEngine->GetButton(KEY_TYPE::A))
		pos -= GetTransform()->GetRight() * _speed * GEngine->GetDeltaTime();

	if (GEngine->GetButton(KEY_TYPE::D))
		pos += GetTransform()->GetRight() * _speed * GEngine->GetDeltaTime();

	if (GEngine->GetButton(KEY_TYPE::Q))
	{
		Vec3 rotation = GetTransform()->GetLocalRotation();
		rotation.x += GEngine->GetDeltaTime() * 0.5f;
		GetTransform()->SetLocalRotation(rotation);
	}

	if (GEngine->GetButton(KEY_TYPE::E))
	{
		Vec3 rotation = GetTransform()->GetLocalRotation();
		rotation.x -= GEngine->GetDeltaTime() * 0.5f;
		GetTransform()->SetLocalRotation(rotation);
	}
	if (GEngine->GetButton(KEY_TYPE::Z))
	{
		Vec3 rotation = GetTransform()->GetLocalRotation();
		rotation.y += GEngine->GetDeltaTime() * 0.5f;
		GetTransform()->SetLocalRotation(rotation);
	}

	if (GEngine->GetButton(KEY_TYPE::C))
	{
		Vec3 rotation = GetTransform()->GetLocalRotation();
		rotation.y -= GEngine->GetDeltaTime() * 0.5f;
		GetTransform()->SetLocalRotation(rotation);
	}
	GetTransform()->SetLocalPosition(pos);
}