#include "pch.h"
#include "RenderController.h"
#include "Engine.h"
#include "ConstantResource.h"
#include "Material.h"
#include "SceneManager.h"
#include "Transform.h"
#include "Light.h"
#include "Resources.h"

RenderController::RenderController()
{
}

RenderController::~RenderController()
{
	::CloseHandle(_fenceEvent);
}

void RenderController::Init()
{
	_viewport = { 0, 0, static_cast<FLOAT>(g_winInfo.width), static_cast<FLOAT>(g_winInfo.height), 0.0f, 1.0f };
	_scissorRect = CD3DX12_RECT(0, 0, g_winInfo.width, g_winInfo.height);

	ResizeWindow(g_winInfo.width, g_winInfo.height);

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
	CreateRenderTargetGroups();
	CreateRootSignature();
	CreateConstant(CBV_REGISTER::b0, sizeof(LightParams), 1);
	CreateConstant(CBV_REGISTER::b1, sizeof(TransformParams), 256);
	CreateConstant(CBV_REGISTER::b2, sizeof(MaterialParams), 256);
	CreateTableDescHeap();
	WaitSync();
}

void RenderController::ResizeWindow(int32 width, int32 height)
{
	g_winInfo.width = width;
	g_winInfo.height = height;

	RECT rect = { 0, 0, width, height };
	::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	::SetWindowPos(g_winInfo.hwnd, 0, 100, 100, width, height, 0);
}

void RenderController::PushDataToCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg)
{
	uint8	regNum = static_cast<uint8>(reg);

	assert(regNum > 0);

	D3D12_CPU_DESCRIPTOR_HANDLE	tableHeapHandle = _tableDescHeap->GetCPUDescriptorHandleForHeapStart();
	uint32	incrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);;

	tableHeapHandle.ptr += (size_t)_currentTableIndex * _tableElementSize;
	tableHeapHandle.ptr += (size_t)(regNum - 1) * incrementSize;

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
	uint32	incrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);;

	tableHeapHandle.ptr += (size_t)_currentTableIndex * _tableElementSize;
	tableHeapHandle.ptr += (size_t)(regNum - 1) * incrementSize;

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

	swap_chain_desc.BufferDesc.Width = static_cast<uint32>(g_winInfo.width);
	swap_chain_desc.BufferDesc.Height = static_cast<uint32>(g_winInfo.height);
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	swap_chain_desc.OutputWindow = g_winInfo.hwnd;
	swap_chain_desc.Windowed = g_winInfo.windowed;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	_dxgi->CreateSwapChain(_cmdQueue.Get(), &swap_chain_desc, &_swapChain);
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
	uint32	handleIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);;;

	tableHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	tableHeapDesc.NumDescriptors = count * (REGISTER_COUNT - 1);
	tableHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	_device->CreateDescriptorHeap(&tableHeapDesc, IID_PPV_ARGS(&_tableDescHeap));

	_tableElementCount = count;
	_tableElementSize = handleIncrementSize * (REGISTER_COUNT - 1);
}

void RenderController::CreateRenderTargetGroups()
{
	shared_ptr<Texture> dsTexture = GET_SINGLE(Resources)->CreateTexture(L"DepthStencil",
		DXGI_FORMAT_D32_FLOAT, g_winInfo.width, g_winInfo.height,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	{
		vector<RenderTarget> rtVec(SWAP_CHAIN_BUFFER_COUNT);

		for (uint32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		{
			wstring name = L"SwapChainTarget_" + std::to_wstring(i);

			ComPtr<ID3D12Resource> resource;
			_swapChain->GetBuffer(i, IID_PPV_ARGS(&resource));
			rtVec[i].target = GET_SINGLE(Resources)->CreateTextureFromResource(name, resource);
		}

		_rtGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)] = make_shared<RenderTargetGroup>();
		_rtGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)]->Create(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN, rtVec, dsTexture);
	}

	{
		vector<RenderTarget> rtVec(RENDER_TARGET_G_BUFFER_GROUP_MEMBER_COUNT);

		rtVec[0].target = GET_SINGLE(Resources)->CreateTexture(L"PositionTarget",
			DXGI_FORMAT_R32G32B32A32_FLOAT, g_winInfo.width, g_winInfo.height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		rtVec[1].target = GET_SINGLE(Resources)->CreateTexture(L"NormalTarget",
			DXGI_FORMAT_R32G32B32A32_FLOAT, g_winInfo.width, g_winInfo.height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		rtVec[2].target = GET_SINGLE(Resources)->CreateTexture(L"DiffuseTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, g_winInfo.width, g_winInfo.height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		_rtGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)] = make_shared<RenderTargetGroup>();
		_rtGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)]->Create(RENDER_TARGET_GROUP_TYPE::G_BUFFER, rtVec, dsTexture);
	}
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
		GEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)->GetRTTexture(_backBufferIndex)->GetTex2D().Get(),
		D3D12_RESOURCE_STATE_PRESENT, // 화면 출력
		D3D12_RESOURCE_STATE_RENDER_TARGET); // 외주 결과물

	_cmdList->ResourceBarrier(1, &barrier);
	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorRect);
}

void	RenderController::RenderEnd()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		GEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)->GetRTTexture(_backBufferIndex)->GetTex2D().Get(),
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
