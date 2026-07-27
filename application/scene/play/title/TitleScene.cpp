#include "TitleScene.h"

// engine/graphics
#include "engine/manager/graphics/ModelManager.h"
#include "engine/graphics/3d/Object3dCommon.h"

// engine/time
#include "engine/time/TimeManager.h"

// scene
#include "engine/scene/manager/SceneManager.h"
#include "engine/scene/factory/SceneFactory.h"
#include "manager/scene/CameraManager.h"
#include "manager/scene/LightManager.h"

#ifdef USE_IMGUI
#include "manager/editor/DebugUIManager.h"
#include "externals/imgui/imgui.h"
#endif

REGISTER_SCENE(TitleScene);

void TitleScene::Initialize()
{
#ifdef USE_IMGUI
    KCE::DebugUIManager::GetInstance()->RegisterDebugUI(this, "Title Scene", [this]() { this->DrawImGui(); }, KCE::DebugUIArea::Hierarchy);
#endif

	KCE::Object3dCommon* objCommon = sceneManager_->GetObject3dCommon();
	KCE::LightManager* lightManager = sceneManager_->GetLightManager();
	KCE::Camera* camera = sceneManager_->GetCameraManager() ? sceneManager_->GetCameraManager()->GetActiveCamera() : nullptr;

	if (objCommon)
	{
		// スカイドーム
		skydome_ = std::make_unique<KCE::Object3d>();
		skydome_->Initialize(objCommon, camera);
		if (lightManager) skydome_->SetLightManager(lightManager);
		skydome_->SetModel("skydome");

		// サンプルキューブ
		sampleCube_ = std::make_unique<KCE::Object3d>();
		sampleCube_->Initialize(objCommon, camera);
		if (lightManager) sampleCube_->SetLightManager(lightManager);
		sampleCube_->SetModel("cube");
		sampleCube_->SetTranslate({ 0.0f, 0.0f, 0.0f });
	}

	timer_ = 0.0f;
}

void TitleScene::OnFinalize()
{
#ifdef USE_IMGUI
    if (KCE::DebugUIManager::HasInstance()) {
        KCE::DebugUIManager::GetInstance()->UnregisterDebugUI(this);
    }
#endif
}

void TitleScene::CommonUpdate()
{
	float dt = KCE::TimeManager::GetInstance().GetGameContext().deltaTime;
	timer_ += dt;

	if (sampleCube_)
	{
		sampleCube_->SetRotate({ 0.0f, timer_ * 0.5f, 0.0f });
		sampleCube_->Update();
	}
	if (skydome_)
	{
		skydome_->Update();
	}
}

void TitleScene::Draw3D()
{
	if (skydome_)
	{
		skydome_->Draw();
	}
	if (sampleCube_)
	{
		sampleCube_->Draw();
	}
}

void TitleScene::Draw2D()
{
}

void TitleScene::DrawImGui()
{
#ifdef USE_IMGUI
    ImGui::Text("Sample Title Scene");
#endif
}

