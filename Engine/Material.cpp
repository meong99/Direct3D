#include "pch.h"
#include "Material.h"

#include "Engine.h"
#include "Shader.h"
#include "Texture.h"

Material::Material() : Object(OBJECT_TYPE::MATERIAL)
{
}

Material::~Material()
{
}

void Material::PushData()
{
	GEngine->GetRenderController()->GetConstantResource(CONSTANT_BUFFER_TYPE::MATERIAL)->PushDataToConstBuffer(&_params, sizeof(_params));

	for (size_t i = 0; i < _textures.size(); i++)
	{
		if (_textures[i] == nullptr)
			continue;

		SRV_REGISTER reg = SRV_REGISTER(static_cast<int8>(SRV_REGISTER::t0) + i);
		GEngine->GetRenderController()->PushDataToSRV(_textures[i]->GetSRVHandle(), reg);
	}
	_shader->Update();
}
