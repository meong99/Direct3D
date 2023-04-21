#pragma once

#include "ConstantResource.h"

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
	DXGI_FORMAT&						GetDSVFormat() { return _dsvFormat; }
	shared_ptr<ConstantResource>		GetConstantResource(CONSTANT_BUFFER_TYPE type) {return _constantResource[static_cast<uint8>(type)];}

private:

	/*
	 *  Initialize
	 */

	void	CreateCommandObject();
	void	CreateSwapChain();
	void	CreateRTV();
	void	CreateDSV();
	void	CreateRootSignature();
	void	CreateConstant(CBV_REGISTER reg, uint32 size, uint32 count);
	void	CreateTableDescHeap();
	
	/*
	 *  Rendering
	 */
	void	RenderBegin();
	void	WaitSync();
	void	RenderEnd();
	void	ClearHeapIndex();

	/*
	 * Access
	 */


	
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
	ComPtr<ID3D12Resource>			_rtvBuffer[SWAP_CHAIN_BUFFER_COUNT];
	ComPtr<ID3D12DescriptorHeap>	_rtvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE		_rtvHeapHandle[SWAP_CHAIN_BUFFER_COUNT];
	uint32							_backBufferIndex = 0;
	uint32							_rtvIncrementSize = 0;

	/*
	 * DSV
	 */
	ComPtr<ID3D12Resource>			_dsvBuffer;
	ComPtr<ID3D12DescriptorHeap>	_dsvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE		_dsvHandle = {};
	DXGI_FORMAT						_dsvFormat = DXGI_FORMAT_D32_FLOAT;

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

};

