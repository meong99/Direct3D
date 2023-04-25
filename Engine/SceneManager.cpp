#include "pch.h"
#include "SceneManager.h"
#include "Engine.h"
#include "Scene.h"
#include "Material.h"
#include "Mesh.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Camera.h"
#include "Transform.h"
#include "Shader.h"
#include "Texture.h"
#include "TestCameraScript.h"
#include "Resources.h"
#include "Light.h"

void SceneManager::Update()
{
	if (_activeScene == nullptr)
		return;

	_activeScene->Update();
	_activeScene->LateUpdate();
	_activeScene->FinalUpdate();
}

void SceneManager::Render()
{
	if (_activeScene)
		_activeScene->Render();
}

void SceneManager::LoadScene(wstring sceneName)
{
	_activeScene = LoadTestScene();

	_activeScene->Awake();
	_activeScene->Start();
}

uint8 SceneManager::LayerNameToIndex(const wstring& name)
{
	auto findIt = _layerIndex.find(name);

	if (findIt == _layerIndex.end())
		return 0;

	return findIt->second;
}

void SceneManager::SetLayerName(uint8 index, const wstring& name)
{
	const wstring& prevName = _layerNames[index];

	_layerIndex.erase(prevName);

	_layerNames[index] = name;
	_layerIndex[name] = index;
}

shared_ptr<Scene> SceneManager::LoadTestScene()
{
#pragma region LayerMask
	SetLayerName(0, L"Default");
	SetLayerName(1, L"UI");
#pragma endregion

	shared_ptr<Scene> scene = make_shared<Scene>();

#pragma region MainCamera
	{
		shared_ptr<GameObject> camera = make_shared<GameObject>();

		camera->SetName(L"Main_Camera");
		camera->AddComponent(make_shared<Transform>());
		camera->AddComponent(make_shared<Camera>());
		camera->AddComponent(make_shared<TestCameraScript>());
		camera->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));

		uint8	layerIndex = GET_SINGLE(SceneManager)->LayerNameToIndex(L"UI");
		camera->GetCamera()->SetCullingMaskLayerOnOff(layerIndex, true);

		scene->AddGameObject(camera);
	}
#pragma endregion

#pragma region UI_Camera
	{
		shared_ptr<GameObject>	camera = make_shared<GameObject>();

		camera->SetName(L"Orthographic_Camera");
		camera->AddComponent(make_shared<Transform>());
		camera->AddComponent(make_shared<Camera>());
		camera->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
		camera->GetCamera()->SetProjectionType(PROJECTION_TYPE::ORTHOGRAPHIC);

		uint8	layerIndex = GET_SINGLE(SceneManager)->LayerNameToIndex(L"UI");
		camera->GetCamera()->SetCullingMaskAll();
		camera->GetCamera()->SetCullingMaskLayerOnOff(layerIndex, false);

		scene->AddGameObject(camera);
	}
#pragma endregion

#pragma region SkyBox
	{
		shared_ptr<GameObject> skybox = make_shared<GameObject>();

		skybox->SetName(L"SkyBox");
		skybox->AddComponent(make_shared<Transform>());
		skybox->SetCheckFrustum(false);

		shared_ptr<MeshRenderer>	meshRenderer = make_shared<MeshRenderer>();
		shared_ptr<Mesh>			sphereMesh = GET_SINGLE(Resources)->LoadSphereMesh();
		shared_ptr<Material>		material = make_shared<Material>();
		shared_ptr<Shader>			shader = GET_SINGLE(Resources)->Get<Shader>(L"Skybox");
		shared_ptr<Texture>			texture = GET_SINGLE(Resources)->Load<Texture>(L"Sky01", L"Resources\\Texture\\Sky01.jpg");

		material->SetShader(shader);
		material->SetTexture(0, texture);
		meshRenderer->SetMesh(sphereMesh);
		meshRenderer->SetMaterial(material);
		skybox->AddComponent(meshRenderer);
		scene->AddGameObject(skybox);
	}
#pragma endregion

#pragma region Cube
	{
		shared_ptr<GameObject> cube = make_shared<GameObject>();

		cube->SetName(L"Cube");
		cube->AddComponent(make_shared<Transform>());
		cube->GetTransform()->SetLocalScale(Vec3(100.f, 100.f, 100.f));
		cube->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 150.f));

		shared_ptr<MeshRenderer>	meshRenderer = make_shared<MeshRenderer>();
		shared_ptr<Mesh>			cubeMesh = GET_SINGLE(Resources)->LoadCubeMesh();
		shared_ptr<Material>		material = make_shared<Material>();
		shared_ptr<Shader>			shader = GET_SINGLE(Resources)->Get<Shader>(L"Deferred");
		shared_ptr<Texture>			texture1 = GET_SINGLE(Resources)->Load<Texture>(L"Brick.jpg", L"Resources\\Texture\\Brick.jpg");
		shared_ptr<Texture>			texture2 = GET_SINGLE(Resources)->Load<Texture>(L"Brick_normal.jpg", L"Resources\\Texture\\Brick_normal.jpg");

		material->SetShader(shader);
		material->SetTexture(0, texture1);
		material->SetTexture(1, texture2);
		meshRenderer->SetMesh(cubeMesh);
		meshRenderer->SetMaterial(material);
		cube->AddComponent(meshRenderer);
		scene->AddGameObject(cube);
	}
#pragma endregion

#pragma region UI_Test
	for (int32 i = 0; i < 3; i++)
	{
		shared_ptr<GameObject> sphere = make_shared<GameObject>();

		sphere->SetName(L"UI_Test");
		sphere->SetLayerIndex(GET_SINGLE(SceneManager)->LayerNameToIndex(L"UI"));
		sphere->AddComponent(make_shared<Transform>());
		sphere->GetTransform()->SetLocalScale(Vec3(100.f, 100.f, 100.f));
		sphere->GetTransform()->SetLocalPosition(Vec3(-350.f + (i * 160), 250.f, 500.f));

		shared_ptr<MeshRenderer>	meshRenderer = make_shared<MeshRenderer>();
		shared_ptr<Mesh>			mesh = GET_SINGLE(Resources)->LoadRectangleMesh();

		meshRenderer->SetMesh(mesh);

		shared_ptr<Shader>		shader = GET_SINGLE(Resources)->Get<Shader>(L"Forward");
		shared_ptr<Texture>		texture = GEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::G_BUFFER)->GetRTTexture(i);
		shared_ptr<Material>	material = make_shared<Material>();

		material->SetShader(shader);
		material->SetTexture(0, texture);
		meshRenderer->SetMaterial(material);
		sphere->AddComponent(meshRenderer);
		scene->AddGameObject(sphere);
	}
#pragma endregion

#pragma region Directional Light
	shared_ptr<GameObject> light = make_shared<GameObject>();

	light->SetName(L"Light");
	light->AddComponent(make_shared<Transform>());
	light->AddComponent(make_shared<Light>());
	light->GetLight()->SetLightDirection(Vec3(1.f, 0.f, 1.f));
	light->GetLight()->SetLightType(LIGHT_TYPE::DIRECTIONAL_LIGHT);
	light->GetLight()->SetDiffuse(Vec3(.5f, .5f, .5f));
	light->GetLight()->SetAmbient(Vec3(0.1f, 0.1f, 0.1f));
	light->GetLight()->SetSpecular(Vec3(0.2f, 0.2f, 0.2f));

	scene->AddGameObject(light);

#pragma endregion

	return scene;
}