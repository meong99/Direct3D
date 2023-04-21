#pragma once

#include "Object.h"

class Shader : public Object
{
public:
	Shader();
	~Shader();

	void	Init(const wstring& path);
	void	Update();

private:
	void	CreateVertexShader(ComPtr<ID3DBlob>& blob, const wstring& path, const string& name, const string& version);
	void	CreateIndexShader(ComPtr<ID3DBlob>& blob, const wstring& path, const string& name, const string& version);
private:

	ComPtr<ID3D12PipelineState>	_pipelineState;
	ComPtr<ID3DBlob>			_vsBlob;
	ComPtr<ID3DBlob>			_psBlob;
};
