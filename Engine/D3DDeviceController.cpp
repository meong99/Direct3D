#include "pch.h"
#include "D3DDeviceController.h"

D3DDeviceController::D3DDeviceController()
{}

D3DDeviceController::~D3DDeviceController()
{
	::CloseHandle(_fenceEvent);
}

void D3DDeviceController::Init(const WinInfo& info)
{
	_winInfo = info;
	_viewport = { 0, 0, static_cast<FLOAT>(info.width), static_cast<FLOAT>(info.height), 0.0f, 1.0f };
	_scissorRect = CD3DX12_RECT(0, 0, info.width, info.height);

	ResizeWindow(info.width, info.height);

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
	CreateConstant();
	CreateTableDescHeap();
	CreateVertex();
	CreateIndex();
	_vsBlob = CreateShader(L"Engine/default.hlsli", "VS_Main", "vs_5_0");
	_psBlob = CreateShader(L"Engine/default.hlsli", "PS_Main", "ps_5_0");
	CreatePOS();

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
	int32 rtvIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		_rtvHeapHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * rtvIncrementSize);
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

void D3DDeviceController::CreateConstant()
{
	_constBufferElementSize = (sizeof(Transform) + 255) & ~255;
	_constBufferElementCount = CONST_BUFFER_COUNT;

	uint32	bufferSize = _constBufferElementSize * _constBufferElementCount;
	D3D12_HEAP_PROPERTIES	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC		resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	_device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &resDesc,
									D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
									IID_PPV_ARGS(&_constBuffer));
	_constBuffer->Map(0, nullptr, reinterpret_cast<void**>(&_constMappedBuffer));

	D3D12_DESCRIPTOR_HEAP_DESC	cbvHeapDesc = {};

	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	cbvHeapDesc.NumDescriptors = _constBufferElementCount;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&_cbvHeap));

	_cbvHandleBegin = _cbvHeap->GetCPUDescriptorHandleForHeapStart();
	_cbvHandleIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (uint32 i = 0; i < _constBufferElementCount; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE		cbvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(_cbvHandleBegin, i * _cbvHandleIncrementSize);
		D3D12_CONSTANT_BUFFER_VIEW_DESC	cbvDesc = {};

		cbvDesc.BufferLocation = _constBuffer->GetGPUVirtualAddress() + static_cast<uint64>(_constBufferElementSize) * i;
		cbvDesc.SizeInBytes = _constBufferElementSize;

		_device->CreateConstantBufferView(&cbvDesc, cbvHandle);
	}
}

void D3DDeviceController::CreateTableDescHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC	tableHeapDesc = {};

	tableHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	tableHeapDesc.NumDescriptors = TABLE_HEAP_COUNT;
	tableHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	_device->CreateDescriptorHeap(&tableHeapDesc, IID_PPV_ARGS(&_tableDescHeap));

	_tableElementCount = TABLE_HEAP_COUNT;
	_tableElementSize = _cbvHandleIncrementSize * TOTAL_REGISTER_COUNT;
}

void D3DDeviceController::CreateVertex()
{
	vector<Vertex> vertex_buffer(4);

	vertex_buffer[0].pos = Vec3(-0.5f, 0.5f, 0.0f);
	vertex_buffer[1].pos = Vec3(0.5f, 0.5f, 0.0f);
	vertex_buffer[2].pos = Vec3(0.5f, -0.5f, 0.0f);
	vertex_buffer[3].pos = Vec3(-0.5f, -0.5f, 0.0f);
	vertex_buffer[0].color = Vec4(1.f, 0.f, 0.f, 1.f);
	vertex_buffer[1].color = Vec4(0.f, 1.f, 0.f, 1.f);
	vertex_buffer[2].color = Vec4(0.f, 0.f, 1.f, 1.f);
	vertex_buffer[3].color = Vec4(1.f, 0.f, 0.f, 1.f);

	_vertexCount = static_cast<uint32>(vertex_buffer.size());
	uint32 bufferSize = _vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc,
									D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
									IID_PPV_ARGS(&_vertexBuffer));
	void* vertexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0);

	_vertexBuffer->Map(0, &readRange, &vertexDataBuffer);
	::memcpy(vertexDataBuffer, &vertex_buffer[0], bufferSize);
	_vertexBuffer->Unmap(0, nullptr);

	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.StrideInBytes = sizeof(Vertex);
	_vertexBufferView.SizeInBytes = bufferSize;
}

void D3DDeviceController::CreateIndex()
{
	vector<uint32> indexBuffer = 
	{
		0, 1, 2,
		0, 2, 3
	};

	_indexCount = static_cast<uint32>(indexBuffer.size());
	uint32 bufferSize = _indexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc,
									D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
									IID_PPV_ARGS(&_indexBuffer));
	void* vertexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0);

	_indexBuffer->Map(0, &readRange, &vertexDataBuffer);
	::memcpy(vertexDataBuffer, &indexBuffer[0], bufferSize);
	_indexBuffer->Unmap(0, nullptr);

	_indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	_indexBufferView.SizeInBytes = bufferSize;
}

ComPtr<ID3DBlob> D3DDeviceController::CreateShader(const wstring& path, const string& name, const string& version)
{
	uint32 compileFlag = 0;

#ifdef _DEBUG
	compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ComPtr<ID3DBlob>	byteCode;
	ComPtr<ID3DBlob>	err;

	HRESULT hr = ::D3DCompileFromFile(path.c_str(), nullptr,
									  D3D_COMPILE_STANDARD_FILE_INCLUDE,
									  name.c_str(), version.c_str(), compileFlag,
									  0, &byteCode, &err);
	if(err != nullptr)
		OutputDebugStringA((char*)err->GetBufferPointer());
	if (FAILED(hr))
		::MessageBoxA(nullptr, "Shader Create Failed !", nullptr, MB_OK);

	return byteCode;
}

void D3DDeviceController::CreatePOS()
{
	D3D12_INPUT_ELEMENT_DESC desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	_pipelineDesc.InputLayout = { desc, _countof(desc) };
	_pipelineDesc.VS = { _vsBlob->GetBufferPointer(), _vsBlob->GetBufferSize() };
	_pipelineDesc.PS = { _psBlob->GetBufferPointer(), _psBlob->GetBufferSize() };
	_pipelineDesc.pRootSignature = _rootSignature.Get();
	_pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	_pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	_pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	_pipelineDesc.DSVFormat = _dsvFormat;
	_pipelineDesc.SampleMask = UINT_MAX;
	_pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	_pipelineDesc.NumRenderTargets = 1;
	_pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	_pipelineDesc.SampleDesc.Count = 1;

	HRESULT hr = _device->CreateGraphicsPipelineState(&_pipelineDesc, IID_PPV_ARGS(&_pipelineState));
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

	static Transform t = {};

	if (GetButton(KEY_TYPE::W))
		t.offset.y += 1.f * _deltaTime;
	if (GetButton(KEY_TYPE::S))
		t.offset.y -= 1.f * _deltaTime;
	if (GetButton(KEY_TYPE::A))
		t.offset.x -= 1.f * _deltaTime;
	if (GetButton(KEY_TYPE::D))
		t.offset.x += 1.f * _deltaTime;

	MeshRender(&t, sizeof(Transform));

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
	_cmdList->SetPipelineState(_pipelineState.Get());
}

void D3DDeviceController::MeshRender(void*	data, uint32 size)
{
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_cmdList->IASetVertexBuffers(0, 1, &_vertexBufferView);
	_cmdList->IASetIndexBuffer(&_indexBufferView);

	CopyDataToConstBuffer(data, size);
	CopyDataToTable();

	_cmdList->DrawIndexedInstanced(_indexCount, 1, 0, 0, 0);
}

void D3DDeviceController::CopyDataToConstBuffer(void* data, uint32 size)
{
	assert(_constBufferCurrentIndex < _constBufferElementCount);
	assert(_constBufferElementSize == ((size + 255) & ~255));

	::memcpy(&_constMappedBuffer[_constBufferCurrentIndex * _constBufferElementSize], data, size);

	D3D12_CPU_DESCRIPTOR_HANDLE	cbvHandle = _cbvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE	tableHeapHandle = _tableDescHeap->GetCPUDescriptorHandleForHeapStart();

	cbvHandle.ptr += (size_t)_constBufferCurrentIndex * _cbvHandleIncrementSize;
	tableHeapHandle.ptr += (size_t)_currentTableIndex * _tableElementSize;

	uint32	destRange = 1;
	uint32	srcRange = 1;

	_device->CopyDescriptors(1, &tableHeapHandle, &destRange, 1, &cbvHandle, &srcRange,
							D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	_constBufferCurrentIndex++;
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
	_currentTableIndex = 0;
	_constBufferCurrentIndex = 0;
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