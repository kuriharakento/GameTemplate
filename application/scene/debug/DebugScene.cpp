#include "DebugScene.h"
#include "gameobject/manager/GameObjectManager.h"
#include "manager/graphics/LineManager.h"
#include "math/VectorColorCodes.h"
#include "scene/factory/SceneFactory.h"
#include "scene/manager/SceneManager.h"

REGISTER_SCENE(DebugScene);

namespace
{
// 床の目安に出すグリッド
constexpr float kGridSize = 40.0f;
constexpr float kGridSpacing = 2.0f;
} // namespace

void DebugScene::Initialize()
{
}

void DebugScene::CommonUpdate()
{
}

void DebugScene::Draw3D()
{
	KCE::GameObjectManager::GetInstance()->Draw3D(sceneManager_->GetCameraManager());
	KCE::LineManager::GetInstance()->DrawGrid(kGridSize, kGridSpacing, KCE::VectorColorCodes::White);
}

void DebugScene::Draw2D()
{
}

void DebugScene::OnFinalize()
{
}
