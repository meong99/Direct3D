#pragma once

class ConstantResource
{
public:
	void	CreateConstant(CBV_REGISTER reg, uint32 size, uint32 count);
	void	ClearIndex();
	void	CopyDataToConstBuffer(void* data, uint32 size);

private:
	/*
	 * Constant
	 */
	ComPtr<ID3D12Resource>			_constBuffer;
	BYTE*							_constMappedBuffer = nullptr;
	uint32							_constBufferElementSize = 0;
	uint32							_constBufferElementCount = 0;
	uint32							_constBufferCurrentIndex = 0;
	ComPtr<ID3D12DescriptorHeap>	_cbvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE		_cbvHandleBegin = {};
	uint32							_cbvHandleIncrementSize = 0;

	CBV_REGISTER	_reg = {};
};

