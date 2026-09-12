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
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/uvChecker.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/black.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/red.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/testSprite.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/white1x1.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/gradationLine.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/gradation.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/circle2.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/flowerfun.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/star.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/skybox.dds", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/numbers.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/fonts/luna_atlas.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/fonts/nico_atlas.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/simplexNoise.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/flameEye.png", KCE::ResourceLifetime::Resident);
	KCE::TextureManager::GetInstance()->LoadTexture("./Resources/lock_on.png", KCE::ResourceLifetime::Resident);
}

void KCE::MyGame::LoadModels()
{
	// =========================
	// エンジン
	// MEMO: エンジンのデフォルトリソースは、エンジン側で使用するため、ユーザーが削除しないように注意すること。
	// =========================
	KCE::ModelManager::GetInstance()->LoadModel("cube", ".obj", KCE::ResourceLifetime::Resident);
	KCE::ModelManager::GetInstance()->LoadModel("skydome", ".obj", KCE::ResourceLifetime::Resident);
	KCE::ModelManager::GetInstance()->LoadModel("plane", ".gltf", KCE::ResourceLifetime::Resident);
}
