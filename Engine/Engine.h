#pragma once

struct WinInfo;
class D3DDeviceController;

class Engine
{
public:
	Engine();
	~Engine();
	void	Init();
	void	Update();

	shared_ptr<D3DDeviceController>&	GetDevice() { return _device; }
private:

	shared_ptr<D3DDeviceController>	_device;
};
