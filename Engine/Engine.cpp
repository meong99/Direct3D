#include "pch.h"
#include "framework.h"
#include "Engine.h"
#include "RenderController.h"

extern WinInfo g_win_info;

Engine::Engine()
{
	_renderController = make_shared<RenderController>();
}

Engine::~Engine()
{
}

void Engine::Init()
{
	_states.resize(KEY_TYPE_COUNT, KEY_STATE::NONE);
	::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&_frequency));
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_prevCount));

	_renderController->Init();
}

void	Engine::Update()
{
	UpdateKey();
	UpdateTimer();
	ShowFPS();
	_renderController->Update();
}


/*
 *
 *
 *
 *		Input Ans Timer
 *
 *
 *
 */

void Engine::UpdateKey()
{
	HWND hwnd = ::GetActiveWindow();

	if (g_win_info.hwnd != hwnd)
	{
		for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
			_states[key] = KEY_STATE::NONE;

		return;
	}

	BYTE asciiKeys[KEY_TYPE_COUNT] = {};

	if (::GetKeyboardState(asciiKeys) == false)
		return;

	for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
	{
		if (asciiKeys[key] & 0x80)
		{
			KEY_STATE& state = _states[key];

			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
				state = KEY_STATE::PRESS;
			else
				state = KEY_STATE::DOWN;
		}
		else
		{
			KEY_STATE& state = _states[key];

			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
				state = KEY_STATE::UP;
			else
				state = KEY_STATE::NONE;
		}
	}
}

void Engine::UpdateTimer()
{
	uint64 currentCount;

	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));

	_deltaTime = (currentCount - _prevCount) / static_cast<float>(_frequency);
	_prevCount = currentCount;
	_frameCount++;
	_frameTime += _deltaTime;

	if (_frameTime > 1.f)
	{
		_fps = static_cast<uint32>(_frameCount / _frameTime);
		_frameTime = 0.f;
		_frameCount = 0;
	}
}

void Engine::ShowFPS()
{
	uint32 fps = GetFps();

	WCHAR text[100] = L"";

	::wsprintf(text, L"FPS : %d", fps);
	::SetWindowText(g_win_info.hwnd, text);
}