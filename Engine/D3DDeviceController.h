#pragma once

/*
 *  Mesh안에 Material이 있어야 하고
 *	Mesh Rendering 할 때 data와 size를 받는데 이 부분이 CONSTANT_BUFFER_TYPE에 맞아야 한다.
 *	즉 Mesh클래스를 별도로 파야한다.
 */

class Material;
class ConstantResource;
class Mesh;

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

enum class CONSTANT_BUFFER_TYPE : uint8
{
	TRANSFORM,
	MATERIAL,
	END
};

enum
{
	CONSTANT_BUFFER_COUNT = static_cast<uint8>(CONSTANT_BUFFER_TYPE::END)
};

class D3DDeviceController
{
public:
	D3DDeviceController();
	~D3DDeviceController();

	void	Init(const WinInfo& win_info);
	void	Update();

private:
	/*
	 *  Initialize
	 */

	void	ResizeWindow(int32 width, int32 height);
	void	CreateCommandObject();
	void	CreateSwapChain();
	void	CreateRTV();
	void	CreateDSV();
	void	CreateRootSignature();
	void	CreateConstant(CBV_REGISTER reg, uint32 size, uint32 count);
	void	CreateTableDescHeap();
	shared_ptr<Material>&	CreateMaterial(ShaderInfo shaderInfo);
	void	CreateMesh(shared_ptr<Material>& material, vector<Vertex> vertex, vector<uint32> index);
	void	CreatePOS();
	
	/*
	 *  Rendering
	 */
	void	RenderBegin();
	void	MeshRender(shared_ptr<Mesh> mesh, void* data, uint32 size);
	void	CopyDataToTable();
	void	WaitSync();
	void	RenderEnd();
	void	ClearHeapIndex();

	/*
	 * Access
	 */
	shared_ptr<ConstantResource>	GetConstantResource(CONSTANT_BUFFER_TYPE type) {return _constantResource[static_cast<uint8>(type)];}

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


	/*
	 * Variable
	 */

	WinInfo			_winInfo;
	D3D12_VIEWPORT	_viewport;
	D3D12_RECT		_scissorRect;
	/*
	 * D3DDeviceController
	 */
	ComPtr<ID3D12Debug>			_debugController;
	ComPtr<IDXGIFactory>		_dxgi;
	ComPtr<ID3D12Device>		_device;

	/*
	 * Command Queue
	 */

	ComPtr<ID3D12CommandQueue>			_cmdQueue;
	ComPtr<ID3D12CommandAllocator>		_cmdAlloc;
	ComPtr<ID3D12GraphicsCommandList>	_cmdList;

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

	/*
	 * Constant
	 */
	vector<shared_ptr<ConstantResource>>	_constantResource;
	uint32									_CBV_SRV_UAV_IncrementSize = 0;

	/*
	 *  TableDescHeap
	 */
	ComPtr<ID3D12DescriptorHeap>	_tableDescHeap;
	uint32							_tableElementSize = 0;
	uint32							_tableElementCount = 0;
	uint32							_currentTableIndex = 0;

	/*
	 * Mesh And Material
	 */
	vector<shared_ptr<Mesh>>		_meshes;
	vector<shared_ptr<Material>>	_materials;
	ComPtr<ID3DBlob>				_vsBlob;
	ComPtr<ID3DBlob>				_psBlob;

	/*
	 *	PipeLine
	 */
	ComPtr<ID3D12PipelineState>			_pipelineState;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC  _pipelineDesc = {};

	/*
	 * Key and Timer
	 */

	uint32	GetFps() { return _fps; }
	float	GetDeltaTime() { return _deltaTime; }

	vector<KEY_STATE>	_states;
	uint64	_frequency = 0;
	uint64	_prevCount = 0;
	float	_deltaTime = 0.f;
	uint32	_frameCount = 0;
	float	_frameTime = 0.f;
	uint32	_fps = 0;
};

