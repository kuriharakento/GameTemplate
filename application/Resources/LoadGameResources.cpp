#include <scene/MyGame.h>
#include <manager/graphics/TextureManager.h>

///=============================================================================
///						アプリケーションで使うリソースの読み込み
///=============================================================================

// NOTE: エンジンでデフォルト使用するリソースもここで読み込んでいる。

void MyGame::LoadTextures()
{
	TextureManager::GetInstance()->LoadTexture("./Resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/black.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/red.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/testSprite.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/white1x1.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/gradationLine.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/gradation.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/flowerfun.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/star.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/skybox.dds");
	TextureManager::GetInstance()->LoadTexture("./Resources/numbers.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/fonts/luna_atlas.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/fonts/nico_atlas.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/simplexNoise.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/flameEye.png");
}

void MyGame::LoadModels()
{
	ModelManager::GetInstance()->LoadModel("cube");
	ModelManager::GetInstance()->LoadModel("skydome");
	ModelManager::GetInstance()->LoadModel("chicken", ".gltf");
	ModelManager::GetInstance()->LoadModel("plane", ".gltf");
}
