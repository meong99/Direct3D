#include "pch.h"
#include "Game.h"
#include "Engine.h"
#include "SceneManager.h"

Game::Game()
{
}

Game::~Game()
{
}

void Game::Init()
{
    GEngine->Init();
	GET_SINGLE(SceneManager)->LoadScene(L"TestScene");
}

void Game::Update()
{
    GEngine->Update();
}
