#pragma once

#include "ConstantResource.h"
#include "RenderTargetGroup.h"

class RenderController
{
public:
	RenderController();
	~RenderController();

	void	Init();
	void	Update();
	void	Render();

	void	ResizeWindow(int32 width, int32 height);
	void	PushDataToCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg);
	void	PushDataToSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, SRV_REGISTER reg);
	void	PushDataToTable();
	void	FlushResourceCommandQueue();

public:

	ComPtr<ID3D12Device>&				GetDevice() { return	_device; }
	ComPtr<ID3D12CommandQueue>&			GetCmdQueue() { return _cmdQueue; }
	ComPtr<ID3D12GraphicsCommandList>&	GetCmdList() { return _cmdList; }
	ComPtr<ID3D12GraphicsCommandList>&	GetResCmdList() { return _resCmdList; }
	ComPtr<ID3D12RootSignature>&		GetRootSig() { return _rootSignature; }
	shared_ptr<ConstantResource>		GetConstantResource(CONSTANT_BUFFER_TYPE type) {return _constantResource[static_cast<uint8>(type)];}
	uint8								GetBackBufferIndex() { return _backBufferIndex; }
	shared_ptr<RenderTargetGroup>		GetRTGroup(RENDER_TARGET_GROUP_TYPE type) { return _rtGroups[static_cast<uint8>(type)]; }

private:

	/*
	 *  Initialize
	 */

	void	CreateCommandObject();
	void	CreateSwapChain();
	void	CreateRootSignature();
	void	CreateConstant(CBV_REGISTER reg, uint32 size, uint32 count);
	void	CreateTableDescHeap();
	void	CreateRenderTargetGroups();
	/*
	 *  Rendering
	 */
	void	RenderBegin();
	void	WaitSync();
	void	RenderEnd();
	void	ClearHeapIndex();

private:

	D3D12_VIEWPORT	_viewport;
	D3D12_RECT		_scissorRect;

	/*
	 * RenderController
	 */
	ComPtr<ID3D12Debug>		_debugController;
	ComPtr<IDXGIFactory>	_dxgi;
	ComPtr<ID3D12Device>	_device;

	/*
	 * Command Queue
	 */

	ComPtr<ID3D12CommandQueue>			_cmdQueue;
	ComPtr<ID3D12CommandAllocator>		_cmdAlloc;
	ComPtr<ID3D12GraphicsCommandList>	_cmdList;
	ComPtr<ID3D12GraphicsCommandList>	_resCmdList;
	ComPtr<ID3D12CommandAllocator>		_resCmdAlloc;

	ComPtr<ID3D12Fence>					_fence;
	uint32								_fenceValue = 0;
	HANDLE								_fenceEvent = INVALID_HANDLE_VALUE;

	/*
	 * SwapChain
	 */
	ComPtr<IDXGISwapChain>			_swapChain;
	uint32							_backBufferIndex = 0;

	/*
	 * Root Signature
	 */
	ComPtr<ID3D12RootSignature>	_rootSignature;
	D3D12_STATIC_SAMPLER_DESC	_samplerDesc = {};

	/*
	 * Constant
	 */
	vector<shared_ptr<ConstantResource>>	_constantResource;
	uint32									_handleIncrementSize = 0;

	/*
	 *  TableDescHeap
	 */
	ComPtr<ID3D12DescriptorHeap>	_tableDescHeap;
	uint32							_tableElementSize = 0;
	uint32							_tableElementCount = 0;
	uint32							_currentTableIndex = 0;

	/*
	 * RenderTargetGroup
	 */
	array<shared_ptr<RenderTargetGroup>, RENDER_TARGET_GROUP_COUNT> _rtGroups;
};

