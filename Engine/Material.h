#pragma once

enum
{
	MATERIAL_INT_COUNT = 5,
	MATERIAL_FLOAT_COUNT = 5,
};

struct MaterialParams
{
	void SetInt(uint8 index, int32 value) { intParams[index] = value; }
	void SetFloat(uint8 index, float value) { floatParams[index] = value; }

	array<int32, MATERIAL_INT_COUNT> intParams;
	array<float, MATERIAL_FLOAT_COUNT> floatParams;
};

class Material
{
public:
	void SetInt(uint8 index, int32 value) { _params.SetInt(index, value); }
	void SetFloat(uint8 index, float value) { _params.SetFloat(index, value); }

	void	CreateShader(ShaderInfo shaderInfo);
	void	PushData();

private:
	void	CreateVertexShader(ComPtr<ID3DBlob>& blob, const wstring& path, const string& name, const string& version);
	void	CreateIndexShader(ComPtr<ID3DBlob>& blob, const wstring& path, const string& name, const string& version);

	MaterialParams				_params = {};
	ComPtr<ID3D12PipelineState>	_pipelineState;
	ComPtr<ID3DBlob>			_vsBlob;
	ComPtr<ID3DBlob>			_psBlob;
};
