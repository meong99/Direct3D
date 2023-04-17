#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Material.h"
#include "Mesh.h"
#include "GameObject.h"
#include "MeshRenderer.h"

void SceneManager::Update()
{
	if (_activeScene == nullptr)
		return;

	_activeScene->Update();
	_activeScene->LateUpdate();
}

void SceneManager::LoadScene(wstring sceneName)
{
	// TODO : ���� Scene ����
	// TODO : ���Ͽ��� Scene ���� �ε�

	_activeScene = LoadTestScene();

	_activeScene->Awake();
	_activeScene->Start();
}

shared_ptr<Scene> SceneManager::LoadTestScene()
{

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

	shared_ptr<Scene>			scene = make_shared<Scene>();
	shared_ptr<GameObject>		gameObject = make_shared<GameObject>();
	ShaderInfo					shaderInfo = {L"Engine/default.hlsli", "VS_Main", "vs_5_0", "PS_Main", "ps_5_0"};
	shared_ptr<MeshRenderer>	meshRenderer = make_shared<MeshRenderer>();
	shared_ptr<Material>		material = make_shared<Material>();
	shared_ptr<Mesh>			mesh =  make_shared<Mesh>();

	material->CreateShader(shaderInfo);
	mesh->CreateVertexBuffer(vertex_buffer);
	mesh->CreateIndexBuffer(indexBuffer);
	meshRenderer->SetMaterial(material);
	meshRenderer->SetMesh(mesh);
	gameObject->Init(); // Transform
	gameObject->AddComponent(meshRenderer);

	scene->AddGameObject(gameObject);

	return scene;
}