#pragma once

#include "Object.h"

class Mesh : public Object
{
public:
	void	Init(const vector<Vertex>& vertexBuffer, const vector<uint32>& indexBuffer);
	void	Render();

public:
	Mesh();
	virtual ~Mesh();

	D3D12_VERTEX_BUFFER_VIEW	GetVertexBufferView() const {return _vertexBufferView;}
	D3D12_INDEX_BUFFER_VIEW		GetIndexBufferView() const {return _indexBufferView;}
	uint32						GetindexCount() const {return _indexCount;}

private:
	void	CreateVertexBuffer(vector<Vertex> vertex);
	void	CreateIndexBuffer(vector<uint32> index);

private:
	ComPtr<ID3D12Resource>		_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	_vertexBufferView = {};
	uint32						_vertexCount = 0;
	ComPtr<ID3D12Resource>		_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW		_indexBufferView = {};
	uint32						_indexCount = 0;
};
