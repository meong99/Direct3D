#pragma once

class Material;

class Mesh
{
public:
	void	SetMaterial(shared_ptr<Material>& material) {_material = material;}

	D3D12_VERTEX_BUFFER_VIEW	GetVertexBufferView() const {return _vertexBufferView;}
	D3D12_INDEX_BUFFER_VIEW	GetIndexBufferView() const {return _indexBufferView;}
	uint32					GetindexCount() const {return _indexCount;}

	void	CreateVertex(vector<Vertex> vertex);
	void	CreateIndex(vector<uint32> index);

private:

	ComPtr<ID3D12Resource>		_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	_vertexBufferView = {};
	uint32						_vertexCount = 0;
	ComPtr<ID3D12Resource>		_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW		_indexBufferView = {};
	uint32						_indexCount = 0;

	shared_ptr<Material>	_material;
};
