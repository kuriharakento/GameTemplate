#include "TestScene.h"
#include <numbers>
#include "engine/gameobject/manager/GameObjectManager.h"
#include "engine/graphics/3d/Object3dCommon.h"
#include "manager/scene/CameraManager.h"
#include "manager/scene/LightManager.h"
#include "scene/manager/SceneManager.h"
#include "manager/editor/GameObjectEditor.h"
#include "externals/imgui/imgui.h"

void TestScene::Initialize()
{
	// カメラの設定
	sceneManager_->GetCameraManager()->GetActiveCamera()->SetTranslate({ 0.0f, 10.0f, 30.0f });
	sceneManager_->GetCameraManager()->GetActiveCamera()->SetRotate({ 0.1f, 0.0f, 0.0f });

	// ライトの調整
	DirectionalLight dirLight = sceneManager_->GetLightManager()->GetDirectionalLight();
	dirLight.direction = kLightDirection;
	dirLight.intensity = kLightIntensity;
	sceneManager_->GetLightManager()->SetDirectionalLight(dirLight);

	// デフォルトライトマネージャーの設定（Object3d描画用）
	sceneManager_->GetObject3dCommon()->SetDefaultLightManager(sceneManager_->GetLightManager());

	// デバッグカメラの初期化
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize(sceneManager_->GetCameraManager()->GetActiveCamera());
	debugCamera_->Start({ 0.0f, 10.0f, -30.0f }, { 0.2f, 0.0f, 0.0f });

	// ゲームオブジェクトマネージャーの初期化
	GameObjectManager::GetInstance()->Initialize();

	// 1. テスト用キューブオブジェクトの作成
	cubeObject_ = std::make_unique<GameObject>("TestCube");
	cubeObject_->SetName("TestCube");
	cubeObject_->Initialize(sceneManager_->GetObject3dCommon(), sceneManager_->GetLightManager());
	cubeObject_->SetModel("cube");
	cubeObject_->SetPosition({ 0.0f, 2.0f, 0.0f });
	cubeObject_->SetScale({ 2.0f, 2.0f, 2.0f });
	GameObjectManager::GetInstance()->Register(cubeObject_.get());

	// 2. 地面プレーンオブジェクトの作成（X軸を回転させて配置）
	groundObject_ = std::make_unique<GameObject>("GroundPlane");
	groundObject_->SetName("GroundPlane");
	groundObject_->Initialize(sceneManager_->GetObject3dCommon(), sceneManager_->GetLightManager());
	groundObject_->SetModel("plane");
	groundObject_->SetPosition({ 0.0f, 0.0f, 0.0f });
	groundObject_->SetRotation({ -std::numbers::pi_v<float> / 2.0f, 0.0f, 0.0f });
	groundObject_->SetScale({ 50.0f, 50.0f, 50.0f });
	if (auto* obj3d = groundObject_->GetObject3d())
	{
		obj3d->SetCastShadow(false);
	}
	GameObjectManager::GetInstance()->Register(groundObject_.get());

	StartState(SceneState::Playing);
}

void TestScene::Finalize()
{
	// 登録されたオブジェクトの登録解除とクリア
	GameObjectManager::GetInstance()->Finalize();
	cubeObject_.reset();
	groundObject_.reset();
	debugCamera_.reset();
}

void TestScene::OnUpdatePlaying()
{
	if (debugCamera_)
	{
		debugCamera_->Update();
	}

	// ゲームオブジェクトマネージャーの更新
	GameObjectManager::GetInstance()->Update();
}

void TestScene::Draw3D()
{
	// ゲームオブジェクトの3D描画
	GameObjectManager::GetInstance()->Draw3D(sceneManager_->GetCameraManager());
}

void TestScene::Draw2D()
{
	// ゲームオブジェクトの2D描画
	GameObjectManager::GetInstance()->Draw2D();
}

void TestScene::DrawShadow()
{
	// ゲームオブジェクトのシャドウ描画
	GameObjectManager::GetInstance()->DrawShadow(sceneManager_->GetCameraManager()->GetActiveCamera());
}

void TestScene::DrawGBuffer()
{
	// ゲームオブジェクトのGBuffer描画
	GameObjectManager::GetInstance()->DrawGBuffer(sceneManager_->GetCameraManager());
}
