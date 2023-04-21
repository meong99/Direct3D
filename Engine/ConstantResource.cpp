#include "pch.h"
#include "ConstantResource.h"
#include "Engine.h"
#include "RenderController.h"

void ConstantResource::CreateConstant(CBV_REGISTER reg, uint32 size, uint32 count)
{
	_reg = reg;
	_constBufferElementSize = (size + 255) & ~255;
	_constBufferElementCount = count;

	uint32	bufferSize = _constBufferElementSize * _constBufferElementCount;
	D3D12_HEAP_PROPERTIES	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC		resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	GEngine->GetDevice()->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &resDesc,
									D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
									IID_PPV_ARGS(&_constBuffer));
	_constBuffer->Map(0, nullptr, reinterpret_cast<void**>(&_constMappedBuffer));

	D3D12_DESCRIPTOR_HEAP_DESC	cbvHeapDesc = {};

	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	cbvHeapDesc.NumDescriptors = _constBufferElementCount;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	GEngine->GetDevice()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&_cbvHeap));

	_cbvHandleBegin = _cbvHeap->GetCPUDescriptorHandleForHeapStart();
	_cbvHandleIncrementSize = GEngine->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (uint32 i = 0; i < _constBufferElementCount; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE		cbvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(_cbvHandleBegin, i * _cbvHandleIncrementSize);
		D3D12_CONSTANT_BUFFER_VIEW_DESC	cbvDesc = {};

		cbvDesc.BufferLocation = _constBuffer->GetGPUVirtualAddress() + static_cast<uint64>(_constBufferElementSize) * i;
		cbvDesc.SizeInBytes = _constBufferElementSize;

		GEngine->GetDevice()->CreateConstantBufferView(&cbvDesc, cbvHandle);
	}
}

void ConstantResource::ClearIndex()
{
	_constBufferCurrentIndex = 0;
}

void ConstantResource::PushDataToConstBuffer(void* data, uint32 size)
{
	assert(_constBufferCurrentIndex < _constBufferElementCount);
	assert(_constBufferElementSize == ((size + 255) & ~255));

	::memcpy(&_constMappedBuffer[_constBufferCurrentIndex * _constBufferElementSize], data, size);

	D3D12_CPU_DESCRIPTOR_HANDLE	cbvHandle = _cbvHeap->GetCPUDescriptorHandleForHeapStart();

	cbvHandle.ptr += (size_t)_constBufferCurrentIndex * _cbvHandleIncrementSize;

	GEngine->GetRenderController()->PushDataToCBV(cbvHandle, _reg);

	_constBufferCurrentIndex++;
}

void ConstantResource::SetGlobalData(void* buffer, uint32 size)
{
	assert(_constBufferElementSize == ((size + 255) & ~255));
	::memcpy(&_constMappedBuffer[0], buffer, size);
	GEngine->GetCmdList()->SetGraphicsRootConstantBufferView(0, GetGpuVirtualAddress(0));
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantResource::GetGpuVirtualAddress(uint32 index)
{
	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = _constBuffer->GetGPUVirtualAddress();

	objCBAddress += index * _constBufferElementSize;

	return objCBAddress;
}