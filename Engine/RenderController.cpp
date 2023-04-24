#include "pch.h"
#include "RenderController.h"
#include "ConstantResource.h"
#include "Material.h"
#include "SceneManager.h"
#include "Transform.h"
#include "Light.h"

RenderController::RenderController()
{
}

RenderController::~RenderController()
{
	::CloseHandle(_fenceEvent);
}

void RenderController::Init()
{
	_viewport = { 0, 0, static_cast<FLOAT>(g_win_info.width), static_cast<FLOAT>(g_win_info.height), 0.0f, 1.0f };
	_scissorRect = CD3DX12_RECT(0, 0, g_win_info.width, g_win_info.height);

	ResizeWindow(g_win_info.width, g_win_info.height);

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
	CreateConstant(CBV_REGISTER::b0, sizeof(LightParams), 1);
	CreateConstant(CBV_REGISTER::b1, sizeof(TransformParams), 256);
	CreateConstant(CBV_REGISTER::b2, sizeof(MaterialParams), 256);
	CreateTableDescHeap();

	WaitSync();
}

void RenderController::ResizeWindow(int32 width, int32 height)
{
	g_win_info.width = width;
	g_win_info.height = height;

	RECT rect = { 0, 0, width, height };
	::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	::SetWindowPos(g_win_info.hwnd, 0, 100, 100, width, height, 0);
}

void RenderController::PushDataToCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg)
{
	uint8	regNum = static_cast<uint8>(reg);

	assert(regNum > 0);

	D3D12_CPU_DESCRIPTOR_HANDLE	tableHeapHandle = _tableDescHeap->GetCPUDescriptorHandleForHeapStart();

	tableHeapHandle.ptr += (size_t)_currentTableIndex * _tableElementSize;
	tableHeapHandle.ptr += (size_t)(regNum - 1) * _handleIncrementSize;

	uint32	destRange = 1;
	uint32	srcRange = 1;

	_device->CopyDescriptors(1, &tableHeapHandle, &destRange, 1, &srcHandle, &srcRange,
							D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RenderController::PushDataToSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, SRV_REGISTER reg)
{
	uint8	regNum = static_cast<uint8>(reg);

	assert(regNum > 0);

	D3D12_CPU_DESCRIPTOR_HANDLE	tableHeapHandle = _tableDescHeap->GetCPUDescriptorHandleForHeapStart();

	tableHeapHandle.ptr += (size_t)_currentTableIndex * _tableElementSize;
	tableHeapHandle.ptr += (size_t)(regNum - 1) * _handleIncrementSize;

	uint32	destRange = 1;
	uint32	srcRange = 1;

	_device->CopyDescriptors(1, &tableHeapHandle, &destRange, 1, &srcHandle, &srcRange,
							D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RenderController::PushDataToTable()
{
	D3D12_GPU_DESCRIPTOR_HANDLE	tableGPUHabndle = _tableDescHeap->GetGPUDescriptorHandleForHeapStart();

	tableGPUHabndle.ptr += (size_t)_currentTableIndex * _tableElementSize;

	_cmdList->SetGraphicsRootDescriptorTable(1, tableGPUHabndle);

	_currentTableIndex++;
}

void RenderController::FlushResourceCommandQueue()
{
	_resCmdList->Close();

	ID3D12CommandList* cmdListArr[] = { _resCmdList.Get() };
	_cmdQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	WaitSync();

	_resCmdAlloc->Reset();
	_resCmdList->Reset(_resCmdAlloc.Get(), nullptr);
}

void RenderController::CreateCommandObject()
{
	D3D12_COMMAND_QUEUE_DESC	cmd_queue_desc = {};

	cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	_device->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&_cmdQueue));
	_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc));
	_device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc.Get(),
								nullptr, IID_PPV_ARGS(&_cmdList));
	_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_resCmdAlloc));
	_device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, _resCmdAlloc.Get(),
								nullptr, IID_PPV_ARGS(&_resCmdList));
	_cmdList->Close();
}

void RenderController::CreateSwapChain()
{
	_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC	swap_chain_desc;

	swap_chain_desc.BufferDesc.Width = static_cast<uint32>(g_win_info.width);
	swap_chain_desc.BufferDesc.Height = static_cast<uint32>(g_win_info.height);
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	swap_chain_desc.OutputWindow = g_win_info.hwnd;
	swap_chain_desc.Windowed = g_win_info.windowed;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	_dxgi->CreateSwapChain(_cmdQueue.Get(), &swap_chain_desc, &_swapChain);

	for (int32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
		_swapChain->GetBuffer(i, IID_PPV_ARGS(&_rtvBuffer[i]));
}

void RenderController::CreateRTV()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;

	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDesc.NodeMask = 0;

	_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&_rtvHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
	_rtvIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_handleIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		_rtvHeapHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * _rtvIncrementSize);
		_device->CreateRenderTargetView(_rtvBuffer[i].Get(), nullptr, _rtvHeapHandle[i]);
	}
}

void RenderController::CreateDSV()
{
	D3D12_HEAP_PROPERTIES	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC		resDesc = CD3DX12_RESOURCE_DESC::Tex2D(_dsvFormat, g_win_info.width, g_win_info.height);

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

void RenderController::CreateRootSignature()
{
	_samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

	CD3DX12_DESCRIPTOR_RANGE	range[] =
	{
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CBV_REGISTER_COUNT - 1, 1),
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRV_REGISTER_COUNT, 0)
	};

	CD3DX12_ROOT_PARAMETER	param[2];
	param[0].InitAsConstantBufferView(static_cast<uint32>(CBV_REGISTER::b0));
	param[1].InitAsDescriptorTable(_countof(range), range);

	D3D12_ROOT_SIGNATURE_DESC sigDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(param), param, 1, &_samplerDesc);
	sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> blobSignature;
	ComPtr<ID3DBlob> blobError;

	::D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature, &blobError);
	_device->CreateRootSignature(0, blobSignature->GetBufferPointer(), blobSignature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
}

void RenderController::CreateConstant(CBV_REGISTER reg, uint32 size, uint32 count)
{
	uint32	typeint = static_cast<uint32>(reg);

	assert(typeint == _constantResource.size());

	shared_ptr<ConstantResource>	buffer = make_shared<ConstantResource>();

	buffer->CreateConstant(reg, size, count);
	_constantResource.push_back(buffer);
}

void RenderController::CreateTableDescHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC	tableHeapDesc = {};
	uint32	count = 5;

	tableHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	tableHeapDesc.NumDescriptors = count * (REGISTER_COUNT - 1);
	tableHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	_device->CreateDescriptorHeap(&tableHeapDesc, IID_PPV_ARGS(&_tableDescHeap));

	_tableElementCount = count;
	_tableElementSize = _handleIncrementSize * (REGISTER_COUNT - 1);
}

void	RenderController::WaitSync()
{
	_fenceValue++;
	_cmdQueue->Signal(_fence.Get(), _fenceValue);

	if (_fence->GetCompletedValue() < _fenceValue)
	{
		_fence->SetEventOnCompletion(_fenceValue, _fenceEvent);
		::WaitForSingleObject(_fenceEvent, INFINITE);
	}
}

void	RenderController::Update()
{
	GET_SINGLE(SceneManager)->Update();
	Render();
}

void RenderController::Render()
{
	RenderBegin();

	GET_SINGLE(SceneManager)->Render();

	RenderEnd();
}

void RenderController::RenderBegin()
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

	_cmdList->ClearRenderTargetView(backBufferView, Colors::Black, 0, nullptr);
	_cmdList->ClearDepthStencilView(_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	_cmdList->OMSetRenderTargets(1, &backBufferView, FALSE, &_dsvHandle);
}

void	RenderController::RenderEnd()
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

void RenderController::ClearHeapIndex()
{
	for (const auto& elem : _constantResource)
		elem->ClearIndex();

	_currentTableIndex = 0;
	_backBufferIndex = (_backBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}
