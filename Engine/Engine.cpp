#include "pch.h"
#include "framework.h"
#include "Engine.h"
#include "D3DDeviceController.h"

Engine::Engine()
{
	_device = make_shared<D3DDeviceController>();
}

Engine::~Engine()
{
}

void Engine::Init()
{
	_device->Init();
}

void	Engine::Update()
{
	_device->Update();
}