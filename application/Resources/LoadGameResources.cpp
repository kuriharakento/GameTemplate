#include <scene/MyGame.h>
#include <manager/graphics/TextureManager.h>

///=============================================================================
///						アプリケーションで使うリソースの読み込み
///=============================================================================

// NOTE: エンジンでデフォルト使用するリソースもここで読み込んでいる。

void MyGame::LoadTextures()
{
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/black.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/red.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/testSprite.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/white1x1.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/gradationLine.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/gradation.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/flowerfun.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/star.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/skybox.dds");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/numbers.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/fonts/luna_atlas.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/fonts/nico_atlas.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/simplexNoise.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/textures/flameEye.png");
}

void MyGame::LoadModels()
{
	ModelManager::GetInstance()->LoadModel("cube");
	ModelManager::GetInstance()->LoadModel("skydome");
	ModelManager::GetInstance()->LoadModel("chicken", ".gltf");
	ModelManager::GetInstance()->LoadModel("plane", ".gltf");
}
