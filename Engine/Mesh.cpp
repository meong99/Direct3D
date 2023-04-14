#include "pch.h"
#include "Mesh.h"

void Mesh::CreateVertex(vector<Vertex> vertex)
{
	_vertexCount = static_cast<uint32>(vertex.size());
	uint32 bufferSize = _vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	g_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc,
									D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
									IID_PPV_ARGS(&_vertexBuffer));
	void* vertexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0);

	_vertexBuffer->Map(0, &readRange, &vertexDataBuffer);
	::memcpy(vertexDataBuffer, &vertex[0], bufferSize);
	_vertexBuffer->Unmap(0, nullptr);

	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.StrideInBytes = sizeof(Vertex);
	_vertexBufferView.SizeInBytes = bufferSize;
}

void Mesh::CreateIndex(vector<uint32> index)
{
	_indexCount = static_cast<uint32>(index.size());
	uint32 bufferSize = _indexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	g_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc,
									D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
									IID_PPV_ARGS(&_indexBuffer));
	void* vertexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0);

	_indexBuffer->Map(0, &readRange, &vertexDataBuffer);
	::memcpy(vertexDataBuffer, &index[0], bufferSize);
	_indexBuffer->Unmap(0, nullptr);

	_indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	_indexBufferView.SizeInBytes = bufferSize;
}
