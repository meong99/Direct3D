#pragma once

/*
 *	Include
 */
#include <windows.h>
#include <tchar.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <filesystem>

#include "d3dx12.h"
#include <d3d12.h>
#include "SimpleMath.h"
#include <wrl.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include "DirectXTex.h"
#include "DirectXTex.inl"

/*
 *	Define
 */

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3dcompiler")

#ifdef _DEBUG
	#pragma comment(lib, "DirectXTex_Debug.lib")
#else
	#pragma comment(lib, "DirectXTex_Release.lib")
#endif
#define MINIMUM_ALLOC_SIZE 256
#define _HAS_STD_BYTE 0


/*
 *	Else
 */

namespace fs = std::filesystem;
using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

using int8		= __int8;
using int16		= __int16;
using int32		= __int32;
using int64		= __int64;
using uint8		= unsigned __int8;
using uint16	= unsigned __int16;
using uint32	= unsigned __int32;
using uint64	= unsigned __int64;
using Vec2		= DirectX::SimpleMath::Vector2;
using Vec3		= DirectX::SimpleMath::Vector3;
using Vec4		= DirectX::SimpleMath::Vector4;
using Matrix	= DirectX::SimpleMath::Matrix;

extern ComPtr<ID3D12Device>		g_device;

struct WinInfo
{
	HWND	hwnd;
	int32	width;
	int32	height;
	bool	windowed;
};

struct Vertex
{
	Vec3 pos;
	Vec4 color;
};

struct Transform
{
	Vec4	offset;
};

enum class CBV_REGISTER
{
	b0,
	b1,

	END
};

enum
{
	SWAP_CHAIN_BUFFER_COUNT = 2,
	CBV_REGISTER_COUNT = CBV_REGISTER::END,
	TOTAL_REGISTER_COUNT = CBV_REGISTER_COUNT,
	TABLE_HEAP_COUNT = TOTAL_REGISTER_COUNT * 5,
};
