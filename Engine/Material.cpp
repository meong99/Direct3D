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
	for (size_t i = 0; i < _textures.size(); i++)
	{
		if (_textures[i] == nullptr)
			continue;

		SRV_REGISTER reg = SRV_REGISTER(static_cast<int8>(SRV_REGISTER::t0) + i);
		GEngine->GetRenderController()->PushDataToSRV(_textures[i]->GetCpuHandle(), reg);
	}
	_shader->Update();
}
