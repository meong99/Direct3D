#pragma once

#include "Object.h"

enum class RASTERIZER_TYPE
{
	CULL_NONE,
	CULL_FRONT,
	CULL_BACK,
	WIREFRAME,
};

enum class DEPTH_STENCIL_TYPE
{
	LESS,
	LESS_EQUAL,
	GREATER,
	GREATER_EQUAL,
};

struct ShaderInfo
{
	RASTERIZER_TYPE rasterizerType = RASTERIZER_TYPE::CULL_BACK;
	DEPTH_STENCIL_TYPE depthStencilType = DEPTH_STENCIL_TYPE::LESS;
};

class Shader : public Object
{
public:
	Shader();
	~Shader();

	void	Init(const wstring& path, ShaderInfo info = ShaderInfo());
	void	Update();

private:
	void	CreateVertexShader(ComPtr<ID3DBlob>& blob, const wstring& path, const string& name, const string& version);
	void	CreateIndexShader(ComPtr<ID3DBlob>& blob, const wstring& path, const string& name, const string& version);
private:

	ComPtr<ID3D12PipelineState>	_pipelineState;
	ComPtr<ID3DBlob>			_vsBlob;
	ComPtr<ID3DBlob>			_psBlob;
};
