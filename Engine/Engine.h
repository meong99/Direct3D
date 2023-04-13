#pragma once

struct WinInfo;
class D3DDeviceController;

class Engine
{
public:
	Engine();
	~Engine();
	void	Init(const WinInfo& win_info);
	void	Updata(const WinInfo& win_info);

private:

	shared_ptr<D3DDeviceController>	_device;
};
