#include "pch.h"
#include "Shader.h"
#include "Engine.h"

Shader::Shader() : Object(OBJECT_TYPE::SHADER)
{
}

Shader::~Shader()
{
}

void Shader::Init(const wstring& path)
{
	CreateVertexShader(_vsBlob, path, "VS_Main", "vs_5_0");
	CreateIndexShader(_psBlob, path, "PS_Main", "ps_5_0");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};

	D3D12_INPUT_ELEMENT_DESC desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	pipelineDesc.InputLayout = { desc, _countof(desc) };
	pipelineDesc.VS = {_vsBlob->GetBufferPointer(), _vsBlob->GetBufferSize()};
	pipelineDesc.PS = {_psBlob->GetBufferPointer(), _psBlob->GetBufferSize()};
	pipelineDesc.pRootSignature = GEngine->GetRootSig().Get();
	pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pipelineDesc.SampleMask = UINT_MAX;
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.NumRenderTargets = 1;
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineDesc.SampleDesc.Count = 1;
	pipelineDesc.DSVFormat =  GEngine->GetDSVFormat();

	HRESULT hr = GEngine->GetDevice()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(_pipelineState.GetAddressOf()));
}

void	Shader::CreateVertexShader(ComPtr<ID3DBlob>& blob, const wstring& path, const string& name, const string& version)
{
	uint32 compileFlag = 0;

#ifdef _DEBUG
	compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ComPtr<ID3DBlob>	err = nullptr;

	HRESULT hr = ::D3DCompileFromFile(path.c_str(), nullptr,
									  D3D_COMPILE_STANDARD_FILE_INCLUDE,
									  name.c_str(), version.c_str(), compileFlag,
									  0, &blob, &err);
	if(err != nullptr)
		OutputDebugStringA((char*)err->GetBufferPointer());
	if (FAILED(hr))
		::MessageBoxA(nullptr, "Shader Create Failed !", nullptr, MB_OK);
}

void	Shader::CreateIndexShader(ComPtr<ID3DBlob>& blob, const wstring& path, const string& name, const string& version)
{
	uint32 compileFlag = 0;

#ifdef _DEBUG
	compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ComPtr<ID3DBlob>	err = nullptr;

	HRESULT hr = ::D3DCompileFromFile(path.c_str(), nullptr,
									  D3D_COMPILE_STANDARD_FILE_INCLUDE,
									  name.c_str(), version.c_str(), compileFlag,
									  0, &blob, &err);
	if(err != nullptr)
		OutputDebugStringA((char*)err->GetBufferPointer());
	if (FAILED(hr))
		::MessageBoxA(nullptr, "Shader Create Failed !", nullptr, MB_OK);
}

void Shader::Update()
{
	GEngine->GetCmdList()->SetPipelineState(_pipelineState.Get());
}
