#pragma once

#include "Object.h"

enum class SHADER_TYPE : uint8
{
	DEFERRED,
	FORWARD,
};

enum class RASTERIZER_TYPE : uint8
{
	CULL_NONE,
	CULL_FRONT,
	CULL_BACK,
	WIREFRAME,
};

enum class DEPTH_STENCIL_TYPE : uint8
{
	LESS,
	LESS_EQUAL,
	GREATER,
	GREATER_EQUAL,
};

struct ShaderInfo
{
	SHADER_TYPE						shaderType = SHADER_TYPE::FORWARD;
	RASTERIZER_TYPE					rasterizerType = RASTERIZER_TYPE::CULL_BACK;
	DEPTH_STENCIL_TYPE				depthStencilType = DEPTH_STENCIL_TYPE::LESS;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE	topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
};

class Shader : public Object
{
public:
	Shader();
	~Shader();

	void	Init(const wstring& path, ShaderInfo info = ShaderInfo());
	void	Update();

public:
	SHADER_TYPE	GetShaderType() { return _info.shaderType; }

private:
	void	CreateVertexShader(ComPtr<ID3DBlob>& blob, const wstring& path, const string& name, const string& version);
	void	CreateIndexShader(ComPtr<ID3DBlob>& blob, const wstring& path, const string& name, const string& version);
private:
	ShaderInfo	_info;

	ComPtr<ID3D12PipelineState>	_pipelineState;
	ComPtr<ID3DBlob>			_vsBlob;
	ComPtr<ID3DBlob>			_psBlob;
};
