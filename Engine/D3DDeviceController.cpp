#include "pch.h"
#include "D3DDeviceController.h"
#include "ConstantResource.h"
#include "SceneManager.h"

extern WinInfo g_win_info;

D3DDeviceController::D3DDeviceController()
{
}

D3DDeviceController::~D3DDeviceController()
{
	::CloseHandle(_fenceEvent);
}

void D3DDeviceController::Init()
{
	_winInfo = g_win_info;
	_viewport = { 0, 0, static_cast<FLOAT>(_winInfo.width), static_cast<FLOAT>(_winInfo.height), 0.0f, 1.0f };
	_scissorRect = CD3DX12_RECT(0, 0, _winInfo.width, _winInfo.height);

	ResizeWindow(_winInfo.width, _winInfo.height);

#ifdef DEBUG
	D3D12GetDebugInterface(IID_PPV_ARGS(&_debug_controller));
	_debug_controller->EnableDebugLayer();
#endif
	::CreateDXGIFactory(IID_PPV_ARGS(&_dxgi));
	::D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device));
	_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	_fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

	CreateCommandObject();
	CreateSwapChain();
	CreateRTV();
	CreateDSV();
	CreateRootSignature();
	CreateConstant(CBV_REGISTER::b0, sizeof(TransformMatrix), 256);
	CreateTableDescHeap();

	_states.resize(KEY_TYPE_COUNT, KEY_STATE::NONE);
	::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&_frequency));
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_prevCount));

	WaitSync();
}

void D3DDeviceController::ResizeWindow(int32 width, int32 height)
{
	_winInfo.width = width;
	_winInfo.height = height;

	RECT rect = { 0, 0, width, height };
	::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	::SetWindowPos(_winInfo.hwnd, 0, 100, 100, width, height, 0);
}

void D3DDeviceController::CreateCommandObject()
{
	D3D12_COMMAND_QUEUE_DESC	cmd_queue_desc = {};

	cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	_device->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&_cmdQueue));
	_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc));
	_device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc.Get(),
								nullptr, IID_PPV_ARGS(&_cmdList));
	_cmdList->Close();
}

void D3DDeviceController::CreateSwapChain()
{
	_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC	swap_chain_desc;

	swap_chain_desc.BufferDesc.Width = static_cast<uint32>(_winInfo.width);
	swap_chain_desc.BufferDesc.Height = static_cast<uint32>(_winInfo.height);
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	swap_chain_desc.OutputWindow = _winInfo.hwnd;
	swap_chain_desc.Windowed = _winInfo.windowed;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	_dxgi->CreateSwapChain(_cmdQueue.Get(), &swap_chain_desc, &_swapChain);

	for (int32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
		_swapChain->GetBuffer(i, IID_PPV_ARGS(&_rtvBuffer[i]));
}

void D3DDeviceController::CreateRTV()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;

	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDesc.NodeMask = 0;

	_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&_rtvHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
	_rtvIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_CBV_SRV_UAV_IncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		_rtvHeapHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * _rtvIncrementSize);
		_device->CreateRenderTargetView(_rtvBuffer[i].Get(), nullptr, _rtvHeapHandle[i]);
	}
}

void D3DDeviceController::CreateDSV()
{
	D3D12_HEAP_PROPERTIES	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC		resDesc = CD3DX12_RESOURCE_DESC::Tex2D(_dsvFormat, _winInfo.width, _winInfo.height);

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE	optimizedClearvalue = CD3DX12_CLEAR_VALUE(_dsvFormat, 1.f, 0);

	_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc,
									D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearvalue,
									IID_PPV_ARGS(&_dsvBuffer));

	D3D12_DESCRIPTOR_HEAP_DESC	dsvHeapDesc = {};

	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap));
	_dsvHandle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_device->CreateDepthStencilView(_dsvBuffer.Get(), nullptr, _dsvHandle);
}

void D3DDeviceController::CreateRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE	range[] =
	{
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CBV_REGISTER_COUNT, 0)
	};

	CD3DX12_ROOT_PARAMETER	param[1];

	param[0].InitAsDescriptorTable(_countof(range), range);

	D3D12_ROOT_SIGNATURE_DESC sigDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(param), param);
	sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> blobSignature;
	ComPtr<ID3DBlob> blobError;

	::D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature, &blobError);
	_device->CreateRootSignature(0, blobSignature->GetBufferPointer(), blobSignature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
}

void D3DDeviceController::CreateConstant(CBV_REGISTER reg, uint32 size, uint32 count)
{
	uint32	typeint = static_cast<uint32>(reg);

	assert(typeint == _constantResource.size());

	shared_ptr<ConstantResource>	buffer = make_shared<ConstantResource>();

	buffer->CreateConstant(reg, size, count);
	_constantResource.push_back(buffer);
}

void D3DDeviceController::CreateTableDescHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC	tableHeapDesc = {};

	tableHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	tableHeapDesc.NumDescriptors = TABLE_HEAP_COUNT;
	tableHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	_device->CreateDescriptorHeap(&tableHeapDesc, IID_PPV_ARGS(&_tableDescHeap));

	_tableElementCount = TABLE_HEAP_COUNT;
	_tableElementSize = _CBV_SRV_UAV_IncrementSize * TOTAL_REGISTER_COUNT;
}

void	D3DDeviceController::WaitSync()
{
	_fenceValue++;
	_cmdQueue->Signal(_fence.Get(), _fenceValue);

	if (_fence->GetCompletedValue() < _fenceValue)
	{
		_fence->SetEventOnCompletion(_fenceValue, _fenceEvent);
		::WaitForSingleObject(_fenceEvent, INFINITE);
	}
}

void	D3DDeviceController::Update()
{
	UpdateKey();
	UpdateTimer();
	ShowFPS();
	RenderBegin();

	GET_SINGLE(SceneManager)->Update();

	RenderEnd();
}

void D3DDeviceController::RenderBegin()
{
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc.Get(), nullptr);
	_cmdList->SetGraphicsRootSignature(_rootSignature.Get());

	ID3D12DescriptorHeap*	tableHeap = _tableDescHeap.Get();

	_cmdList->SetDescriptorHeaps(1, &tableHeap);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_rtvBuffer[_backBufferIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, // 화면 출력
		D3D12_RESOURCE_STATE_RENDER_TARGET); // 외주 결과물

	_cmdList->ResourceBarrier(1, &barrier);
	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = _rtvHeapHandle[_backBufferIndex];

	_cmdList->ClearRenderTargetView(backBufferView, Colors::LightSteelBlue, 0, nullptr);
	_cmdList->ClearDepthStencilView(_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	_cmdList->OMSetRenderTargets(1, &backBufferView, FALSE, &_dsvHandle);
}

void D3DDeviceController::CopyDataToTable()
{
	D3D12_GPU_DESCRIPTOR_HANDLE	tableGPUHabndle = _tableDescHeap->GetGPUDescriptorHandleForHeapStart();

	tableGPUHabndle.ptr += (size_t)_currentTableIndex * _tableElementSize;

	_cmdList->SetGraphicsRootDescriptorTable(0, tableGPUHabndle);

	_currentTableIndex++;
}

void	D3DDeviceController::RenderEnd()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_rtvBuffer[_backBufferIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	_cmdList->ResourceBarrier(1, &barrier);
	_cmdList->Close();

	ID3D12CommandList* cmdListArr[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	_swapChain->Present(0, 0);
	WaitSync();
	ClearHeapIndex();
}

void D3DDeviceController::ClearHeapIndex()
{
	for (const auto& elem : _constantResource)
		elem->ClearIndex();

	_currentTableIndex = 0;
	_backBufferIndex = (_backBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}

void D3DDeviceController::UpdateKey()
{
	HWND hwnd = ::GetActiveWindow();

	if (_winInfo.hwnd != hwnd)
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

void D3DDeviceController::UpdateTimer()
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

void D3DDeviceController::ShowFPS()
{
	uint32 fps = GetFps();

	WCHAR text[100] = L"";

	::wsprintf(text, L"FPS : %d", fps);
	::SetWindowText(_winInfo.hwnd, text);
}