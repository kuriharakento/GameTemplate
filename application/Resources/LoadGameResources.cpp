#include <scene/MyGame.h>
#include <manager/graphics/TextureManager.h>

///=============================================================================
///						アプリケーションで使うリソースの読み込み
///=============================================================================

// NOTE: エンジンでデフォルト使用するリソースもここで読み込んでいる。

void MyGame::LoadTextures()
{
	// =========================
	// エンジン
	// MEMO: エンジンのデフォルトリソースは、エンジン側で使用するため、ユーザーが削除しないように注意すること。
	// =========================
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

	// ===================
	// ゲーム固有のリソース
	// ===================
}

void MyGame::LoadModels()
{
	// =========================
	// エンジン
	// MEMO: エンジンのデフォルトリソースは、エンジン側で使用するため、ユーザーが削除しないように注意すること。
	// =========================
	ModelManager::GetInstance()->LoadModel("cube");
	ModelManager::GetInstance()->LoadModel("skydome");
	ModelManager::GetInstance()->LoadModel("plane", ".gltf");

	// ====================
	// ゲーム固有のリソース	
	// ====================
}
