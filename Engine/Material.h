#pragma once

#include "Object.h"

class Shader;
class Texture;

enum
{
	MATERIAL_INT_COUNT = 5,
	MATERIAL_FLOAT_COUNT = 5,
	MATERIAL_TEXTURE_COUNT = 5,
};

struct MaterialParams
{
	void SetInt(uint8 index, int32 value) { intParams[index] = value; }
	void SetFloat(uint8 index, float value) { floatParams[index] = value; }
	void SetTexOn(uint8 index, int32 value) { texOnParams[index] = value; }

	array<int32, MATERIAL_INT_COUNT> intParams;
	array<float, MATERIAL_FLOAT_COUNT> floatParams;
	array<int32, MATERIAL_TEXTURE_COUNT> texOnParams;
};

class Material : public Object
{
public:
	void	PushData();

public:
	Material();
	virtual ~Material();

	shared_ptr<Shader>	GetShader() { return _shader; }

	void	SetInt(uint8 index, int32 value) { _params.SetInt(index, value); }
	void	SetFloat(uint8 index, float value) { _params.SetFloat(index, value); }
	void	SetShader(shared_ptr<Shader> shader) { _shader = shader; }
	void	SetTexture(uint8 index, shared_ptr<Texture> texture)
	{
		_textures[index] = texture;
		_params.SetTexOn(index, (texture == nullptr ? 0 : 1));
	}

private:
	array<shared_ptr<Texture>, MATERIAL_TEXTURE_COUNT> _textures;
	shared_ptr<Shader>	_shader = nullptr;
	MaterialParams		_params = {};
};
