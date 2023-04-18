#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Material.h"
#include "Mesh.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Camera.h"
#include "Transform.h"
#include "TestCameraScript.h"

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
	if (_activeScene == nullptr)
		return;

	const vector<shared_ptr<GameObject>>& gameObjects = _activeScene->GetGameObjects();
	for (auto& gameObject : gameObjects)
	{
		if (gameObject->GetCamera() == nullptr)
			continue;

		gameObject->GetCamera()->Render();
	}
}

void SceneManager::LoadScene(wstring sceneName)
{
	// TODO : 기존 Scene 정리
	// TODO : 파일에서 Scene 정보 로드

	_activeScene = LoadTestScene();

	_activeScene->Awake();
	_activeScene->Start();
}

shared_ptr<Scene> SceneManager::LoadTestScene()
{

#pragma region
	shared_ptr<Scene>			scene = make_shared<Scene>();

	vector<Vertex> vertex_buffer(4);
	vertex_buffer[0].pos = Vec3(-0.5f, 0.5f, 0.0f);
	vertex_buffer[1].pos = Vec3(0.5f, 0.5f, 0.0f);
	vertex_buffer[2].pos = Vec3(0.5f, -0.5f, 0.0f);
	vertex_buffer[3].pos = Vec3(-0.5f, -0.5f, 0.0f);
	vertex_buffer[0].color = Vec4(1.f, 0.f, 0.f, 1.f);
	vertex_buffer[1].color = Vec4(0.f, 1.f, 0.f, 1.f);
	vertex_buffer[2].color = Vec4(0.f, 0.f, 1.f, 1.f);
	vertex_buffer[3].color = Vec4(1.f, 0.f, 0.f, 1.f);
	vector<uint32> indexBuffer = 
	{
		0, 1, 2,
		0, 2, 3
	};
	shared_ptr<GameObject>		gameObject = make_shared<GameObject>();
	ShaderInfo					shaderInfo = {L"Engine/default.hlsli", "VS_Main", "vs_5_0", "PS_Main", "ps_5_0"};
	shared_ptr<MeshRenderer>	meshRenderer = make_shared<MeshRenderer>();
	shared_ptr<Material>		material = make_shared<Material>();
	shared_ptr<Mesh>			mesh =  make_shared<Mesh>();
	shared_ptr<Transform>		transform;

	gameObject->AddComponent(make_shared<Transform>());
	transform = gameObject->GetTransform();
	transform->SetLocalPosition(Vec3(0.f, 100.f, 200.f));
	transform->SetLocalScale(Vec3(100.f, 100.f, 1.f));

	material->CreateShader(shaderInfo);
	mesh->CreateVertexBuffer(vertex_buffer);
	mesh->CreateIndexBuffer(indexBuffer);
	meshRenderer->SetMaterial(material);
	meshRenderer->SetMesh(mesh);
	gameObject->AddComponent(meshRenderer);

	scene->AddGameObject(gameObject);
#pragma endregion

#pragma region Camera
	shared_ptr<GameObject>	camera = make_shared<GameObject>();

	camera->AddComponent(make_shared<Transform>());
	camera->AddComponent(make_shared<Camera>());
	camera->AddComponent(make_shared<TestCameraScript>());
	camera->GetTransform()->SetLocalPosition(Vec3(0.f, 100.f, 0.f));

	scene->AddGameObject(camera);
#pragma endregion
	return scene;
}