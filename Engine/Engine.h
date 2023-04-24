#pragma once
#include "RenderController.h"

struct	WinInfo;
class	RenderController;

enum class KEY_TYPE
{
	UP = VK_UP,
	DOWN = VK_DOWN,
	LEFT = VK_LEFT,
	RIGHT = VK_RIGHT,

	W = 'W',
	A = 'A',
	S = 'S',
	D = 'D',
	Q = 'Q',
	E = 'E',
	Z = 'Z',
	C = 'C',
};

enum class KEY_STATE
{
	NONE,
	PRESS,
	DOWN,
	UP,
	END
};

enum
{
	KEY_TYPE_COUNT = static_cast<int32>(UINT8_MAX + 1),
	KEY_STATE_COUNT = static_cast<int32>(KEY_STATE::END),
};


class Engine
{
public:
	void	Init();
	void	Update();

public:
	Engine();
	~Engine();

	shared_ptr<RenderController>&		GetRenderController() { return _renderController; }
	ComPtr<ID3D12Device>&				GetDevice() { return	_renderController->GetDevice(); }
	ComPtr<ID3D12CommandQueue>&			GetCmdQueue() { return _renderController->GetCmdQueue(); }
	ComPtr<ID3D12GraphicsCommandList>&	GetCmdList() { return _renderController->GetCmdList(); }
	ComPtr<ID3D12GraphicsCommandList>&	GetResCmdList() { return _renderController->GetResCmdList(); }
	ComPtr<ID3D12RootSignature>&		GetRootSig() { return _renderController->GetRootSig(); }
	DXGI_FORMAT&						GetDSVFormat() { return _renderController->GetDSVFormat(); }
	
	/*
	 *  Key and Timer
	 */
	void				UpdateKey();
	void				UpdateTimer();
	void				ShowFPS();
	bool				GetButton(KEY_TYPE key) { return GetState(key) == KEY_STATE::PRESS; }
	bool				GetButtonDown(KEY_TYPE key) { return GetState(key) == KEY_STATE::DOWN; }
	bool				GetButtonUp(KEY_TYPE key) { return GetState(key) == KEY_STATE::UP; }
	inline KEY_STATE	GetState(KEY_TYPE key) { return _states[static_cast<uint8>(key)]; }
	uint32				GetFps() { return _fps; }
	float				GetDeltaTime() { return _deltaTime; }
private:
	shared_ptr<RenderController>	_renderController;
	
	/*
	 * Key and Timer
	 */
	vector<KEY_STATE>	_states;
	uint64	_frequency = 0;
	uint64	_prevCount = 0;
	float	_deltaTime = 0.f;
	uint32	_frameCount = 0;
	float	_frameTime = 0.f;
	uint32	_fps = 0;
};
