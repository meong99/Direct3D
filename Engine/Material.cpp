#include "pch.h"
#include "Material.h"

ComPtr<ID3DBlob> Material::CreateShader(const wstring& path, const string& name, const string& version)
{
	uint32 compileFlag = 0;

#ifdef _DEBUG
	compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ComPtr<ID3DBlob>	byteCode;
	ComPtr<ID3DBlob>	err;

	HRESULT hr = ::D3DCompileFromFile(path.c_str(), nullptr,
									  D3D_COMPILE_STANDARD_FILE_INCLUDE,
									  name.c_str(), version.c_str(), compileFlag,
									  0, &byteCode, &err);
	if(err != nullptr)
		OutputDebugStringA((char*)err->GetBufferPointer());
	if (FAILED(hr))
		::MessageBoxA(nullptr, "Shader Create Failed !", nullptr, MB_OK);

	return byteCode;
}
