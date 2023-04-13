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

void Engine::Init(const WinInfo& win_info)
{
	_device->Init(win_info);
}

void	Engine::Updata(const WinInfo& win_info)
{
	_device->Update();
}