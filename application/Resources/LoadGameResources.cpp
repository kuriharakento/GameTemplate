#include "MyGame.h"
#include <string>
#include <vector>
#include <manager/graphics/TextureManager.h>

///=============================================================================
///						アプリケーションで使うリソースの読み込み
///=============================================================================

// NOTE: エンジンでデフォルト使用するリソースもここで読み込んでいる。
// NOTE: ファイルの展開はワーカーで並べて回す。SRV の番号はリストの並び順で振られる。

void KCE::MyGame::LoadTextures()
{
	// =========================
	// エンジン
	// MEMO: エンジンのデフォルトリソースは、エンジン側で使用するため、ユーザーが削除しないように注意すること。
	// =========================
	const std::vector<std::string> engineTextures = {
		"./Resources/uvChecker.png",
		"./Resources/black.png",
		"./Resources/red.png",
		"./Resources/testSprite.png",
		"./Resources/white1x1.png",
		"./Resources/gradationLine.png",
		"./Resources/gradation.png",
		"./Resources/circle2.png",
		"./Resources/flowerfun.png",
		"./Resources/star.png",
		"./Resources/skybox.dds",
		"./Resources/numbers.png",
		"./Resources/fonts/luna_atlas.png",
		"./Resources/fonts/nico_atlas.png",
		"./Resources/simplexNoise.png",
		"./Resources/flameEye.png",
		"./Resources/lock_on.png",
	};
	KCE::TextureManager::GetInstance()->LoadTextures(engineTextures, KCE::ResourceLifetime::Resident, jobSystem_.get());
}

void KCE::MyGame::LoadModels()
{
	// =========================
	// エンジン
	// MEMO: エンジンのデフォルトリソースは、エンジン側で使用するため、ユーザーが削除しないように注意すること。
	// =========================
	const std::vector<KCE::ModelManager::ModelRequest> engineModels = {
		{ "cube", ".obj" },
		{ "skydome", ".obj" },
		{ "plane", ".gltf" },
	};
	KCE::ModelManager::GetInstance()->LoadModels(engineModels, KCE::ResourceLifetime::Resident, jobSystem_.get());
}
