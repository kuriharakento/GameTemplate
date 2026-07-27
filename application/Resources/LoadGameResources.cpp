#include "MyGame.h"
#include <manager/graphics/TextureManager.h>

///=============================================================================
///						アプリケーションで使うリソースの読み込み
///=============================================================================

// NOTE: エンジンでデフォルト使用するリソースもここで読み込んでいる。

void KCE::MyGame::LoadTextures()
{
	// =========================
	// エンジン
	// MEMO: エンジンのデフォルトリソースは、エンジン側で使用するため、ユーザーが削除しないように注意すること。
	// =========================
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/uvChecker.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/black.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/red.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/testSprite.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/white1x1.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/gradationLine.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/gradation.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/circle2.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/flowerfun.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/star.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/skybox.dds");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/numbers.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/fonts/luna_atlas.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/fonts/nico_atlas.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/simplexNoise.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/flameEye.png");
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/lock_on.png");
}

void KCE::MyGame::LoadModels()
{
	// =========================
	// エンジン
	// MEMO: エンジンのデフォルトリソースは、エンジン側で使用するため、ユーザーが削除しないように注意すること。
	// =========================
	KCE::ModelManager::GetInstance()->LoadModel("cube");
	KCE::ModelManager::GetInstance()->LoadModel("skydome");
	KCE::ModelManager::GetInstance()->LoadModel("plane", ".gltf");
}
