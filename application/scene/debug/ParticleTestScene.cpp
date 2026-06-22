#include "ParticleTestScene.h"

#include <numbers>

#include "effects/particle/ParticleManager.h"
#include "effects/particle/renderer/SpriteRenderer.h"
#include "effects/particle/module/spawn/SpawnModules.h"
#include "effects/particle/module/spawn/InitialModules.h"
#include "effects/particle/module/update/UpdateModules.h"
#include "manager/graphics/LineManager.h"
#include "manager/scene/CameraManager.h"
#include "math/VectorColorCodes.h"
#include "scene/manager/SceneManager.h"
#include "base/DirectXCommon.h"
#include "manager/system/SrvManager.h"
#include "manager/scene/LightManager.h"


void ParticleTestScene::Initialize()
{
	// カメラの設定
	sceneManager_->GetCameraManager()->GetActiveCamera()->SetTranslate({ 0.0f, 5.0f, 20.0f });
	sceneManager_->GetCameraManager()->GetActiveCamera()->SetRotate({ 0.0f, 0.0f, 0.0f });

	// ディレクショナルライトの調整（斜め下向き）
	DirectionalLight dirLight = sceneManager_->GetLightManager()->GetDirectionalLight();
	dirLight.direction = kLightDirection;
	dirLight.intensity = kLightIntensity;
	sceneManager_->GetLightManager()->SetDirectionalLight(dirLight);

	// デバッグカメラの初期化
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize(sceneManager_->GetCameraManager()->GetActiveCamera());
	debugCamera_->Start({ 0.0f, 8.0f, -30.0f }, { 0.2f, 0.0f, 0.0f });

	// パーティクルエディタの初期化
	particleEditor_ = std::make_unique<ParticleEditor>();
	particleEditor_->Initialize(
		ParticleManager::GetInstance()->GetDxCommon(),
		ParticleManager::GetInstance()->GetSrvManager()
	);
	particleEditor_->SetVisible(true);  // 最初から表示

	// スカイドームの初期化
	skydome_ = std::make_unique<Object3d>();
	skydome_->Initialize(sceneManager_->GetObject3dCommon());
	skydome_->SetModel("skydome");
	skydome_->SetLightManager(sceneManager_->GetLightManager());
	skydome_->SetEnableLighting(true);
	skydome_->SetDirectionalLightIntensity(0.5f);
	skydome_->SetDirectionalLightDirection({ 0.0f, -1.0f, 0.0f });
	skydome_->SetScale({ 0.8f, 0.8f, 0.8f });
	skydome_->SetCastShadow(false);
	
	StartState(SceneState::Playing);
}

void ParticleTestScene::Finalize()
{
	particleEditor_.reset();
	
	// シーン終了時にパーティクルをクリア
	ParticleManager::GetInstance()->Clear();
}

// ==================================================
// 状態フック
// ==================================================

void ParticleTestScene::OnEnterPlaying()
{
	auto* cylinder = ParticleManager::GetInstance()->GetEmitter("auraCylinder");
	auto* mist = ParticleManager::GetInstance()->GetEmitter("auraMist");
	auto* floor = ParticleManager::GetInstance()->GetEmitter("auraFloor");

	if (cylinder) cylinder->SetPosition(Vector3(0.0f, 10.0f, 0.0f));
	if (mist) mist->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
	if (floor) floor->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
}

void ParticleTestScene::OnUpdatePlaying()
{
	if (debugCamera_)
	{
		debugCamera_->Update();
	}

	// エディタの更新（ImGui描画）
	if (particleEditor_)
	{
		particleEditor_->Update(sceneManager_->GetCameraManager());
	}

	// スカイドームの更新
	if (skydome_)
	{
		skydome_->Update(sceneManager_->GetCameraManager());
	}
}

void ParticleTestScene::Draw2D()
{
}

void ParticleTestScene::Draw3D()
{
	LineManager::GetInstance()->DrawGrid(
		50.0f,
		5.0f,
		VectorColorCodes::White
	);

	// スカイドームの描画
	if (skydome_)
	{
		skydome_->Draw();
	}

	// パーティクルエディタのデバッグ描画（エミッター・パーティクル位置表示）
	if (particleEditor_)
	{
		particleEditor_->DrawDebug();
	}
}