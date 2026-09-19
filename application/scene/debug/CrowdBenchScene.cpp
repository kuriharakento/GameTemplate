#include "CrowdBenchScene.h"

#include <chrono>
#include <cmath>
#include <fstream>

#include "base/Camera.h"
#include "ecs/components/InstancedRenderComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/system/InstancedRenderSystem.h"
#include "engine/scene/factory/SceneFactory.h"
#include "gameobject/manager/GameObjectManager.h"
#include "graphics/3d/Object3dCommon.h"
#include "manager/graphics/ModelManager.h"
#include "manager/graphics/ShadowMapManager.h"
#include "manager/scene/CameraManager.h"
#include "manager/scene/LightManager.h"
#include "math/MatrixFunc.h"
#include "scene/manager/SceneManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

REGISTER_SCENE(CrowdBenchScene);

namespace
{
// 見渡せる位置
constexpr KCE::Vector3 kCameraPosition = { 0.0f, 45.0f, -80.0f };
constexpr KCE::Vector3 kCameraRotation = { 0.45f, 0.0f, 0.0f };
// 床の大きさと高さ
constexpr KCE::Vector3 kFloorScale = { 140.0f, 1.0f, 140.0f };
constexpr KCE::Vector3 kFloorPosition = { 0.0f, 0.0f, 0.0f };
// ライト
constexpr KCE::Vector3 kLightDirection = { 0.3f, -1.0f, 0.4f };
constexpr float kLightIntensity = 1.0f;
// 敵を浮かせる高さ
constexpr float kEnemyHeight = 1.0f;
} // namespace

void CrowdBenchScene::Initialize()
{
	if (KCE::Camera* camera = sceneManager_->GetCameraManager()->GetActiveCamera())
	{
		camera->SetTranslate(kCameraPosition);
		camera->SetRotate(kCameraRotation);
	}

	KCE::DirectionalLight light = sceneManager_->GetLightManager()->GetDirectionalLight();
	light.direction = kLightDirection;
	light.intensity = kLightIntensity;
	sceneManager_->GetLightManager()->SetDirectionalLight(light);

	// 床はシーンが直接持つ。敵の数だけを比べたいので GameObjectManager には登録しない
	floor_ = std::make_unique<KCE::Object3d>();
	floor_->Initialize(sceneManager_->GetObject3dCommon());
	floor_->SetModel(kFloorModel);
	floor_->SetLightManager(sceneManager_->GetLightManager());
	floor_->SetScale(kFloorScale);
	floor_->SetTranslate(kFloorPosition);
	floor_->SetCastShadow(false);
	RegisterObject(floor_.get());

	registry_.Initialize(kMaxEntities);
	registry_.RegisterComponent<KCE::ecs::TransformComponent>(kMaxEntities);
	registry_.RegisterComponent<KCE::ecs::InstancedRenderComponent>(kMaxEntities);
	registryReady_ = true;

	rebuildRequest_ = true;
}

void CrowdBenchScene::OnFinalize()
{
	ClearCrowd();
	ecsRenderers_.clear();
	ClearObjects();
	floor_.reset();
}

void CrowdBenchScene::GetCrowdTransform(int index, float time, KCE::Vector3& outPosition, float& outRotationY) const
{
	// 決まった式だけで作る。何度作り直しても同じ動きになる
	const int row = index / kPerRow;
	const int column = index % kPerRow;
	const float baseX = (static_cast<float>(column) - static_cast<float>(kPerRow) * 0.5f) * kSpacing;
	const float baseZ = (static_cast<float>(row) - static_cast<float>(kPerRow) * 0.5f) * kSpacing;
	// 体ごとに動き始めをずらす
	const float phase = static_cast<float>(index) * 0.37f;
	outPosition = {
		baseX + std::sin(time * kMoveSpeed + phase) * kMoveRadius,
		kEnemyHeight,
		baseZ + std::cos(time * kMoveSpeed + phase) * kMoveRadius,
	};
	outRotationY = time * kSpinSpeed + phase;
}

void CrowdBenchScene::ClearCrowd()
{
	auto* manager = KCE::GameObjectManager::GetInstance();
	for (auto& object : gameObjects_)
	{
		if (object)
		{
			manager->Unregister(object.get());
		}
	}
	gameObjects_.clear();

	if (registryReady_)
	{
		for (const KCE::EntityID entity : entities_)
		{
			registry_.DestroyEntityDeferred(entity);
		}
		registry_.FlushGarbageCollection();
	}
	entities_.clear();
}

void CrowdBenchScene::BuildGameObjects()
{
	auto* manager = KCE::GameObjectManager::GetInstance();
	gameObjects_.reserve(static_cast<size_t>(count_));
	for (int i = 0; i < count_; ++i)
	{
		auto object = std::make_unique<KCE::GameObject>("CrowdEnemy");
		object->Initialize(sceneManager_->GetObject3dCommon(), sceneManager_->GetLightManager());
		object->SetName("CrowdEnemy");
		object->SetModel(kEnemyModel);
		object->SetScale({ kEnemyScale, kEnemyScale, kEnemyScale });
		KCE::Vector3 position;
		float rotationY = 0.0f;
		GetCrowdTransform(i, 0.0f, position, rotationY);
		object->SetPosition(position);
		object->SetRotation({ 0.0f, rotationY, 0.0f });
		manager->Register(object.get());
		gameObjects_.push_back(std::move(object));
	}
}

void CrowdBenchScene::BuildEntities()
{
	// まとめ描き用のバッファはモデルごとに1つ。体数を変えても作り直さない
	if (ecsRenderers_.find(kEnemyModel) == ecsRenderers_.end())
	{
		KCE::Object3dCommon* common = sceneManager_->GetObject3dCommon();
		KCE::Model* model = KCE::ModelManager::GetInstance()->FindModel(kEnemyModel);
		if (!common || !model)
		{
			return;
		}
		auto renderer = std::make_unique<KCE::InstancedModelRenderer>(kMaxEntities);
		renderer->Initialize(common->GetDXCommon(), common->GetSrvManager(), model);
		ecsRenderers_.emplace(kEnemyModel, std::move(renderer));
	}

	entities_.reserve(static_cast<size_t>(count_));
	for (int i = 0; i < count_; ++i)
	{
		const KCE::EntityID entity = registry_.CreateEntity();
		if (entity == KCE::kInvalidEntity)
		{
			break;
		}
		KCE::ecs::TransformComponent transform;
		float rotationY = 0.0f;
		GetCrowdTransform(i, 0.0f, transform.localPosition_, rotationY);
		transform.localRotation_ = { 0.0f, rotationY, 0.0f };
		transform.localScale_ = { kEnemyScale, kEnemyScale, kEnemyScale };
		transform.worldMatrix_ = KCE::MakeAffineMatrix(transform.localScale_, transform.localRotation_, transform.localPosition_);
		registry_.AddComponent(entity, transform);

		KCE::ecs::InstancedRenderComponent render;
		render.modelName_ = kEnemyModel;
		registry_.AddComponent(entity, render);

		entities_.push_back(entity);
	}
}

void CrowdBenchScene::Rebuild()
{
	ClearCrowd();
	if (path_ == Path::GameObject)
	{
		BuildGameObjects();
	}
	else
	{
		BuildEntities();
	}
	// 作り直したら測り直す
	frameMsTotal_ = 0.0;
	frameMsMax_ = 0.0;
	frameSamples_ = 0;
	rebuildRequest_ = false;
}

void CrowdBenchScene::CommonUpdate()
{
	if (rebuildRequest_)
	{
		Rebuild();
	}

	// フレーム時間を測る。UI と書き出しはこの数字を使う
	using clock = std::chrono::steady_clock;
	static clock::time_point previous = clock::now();
	const clock::time_point now = clock::now();
	const double frameMs = std::chrono::duration<double, std::milli>(now - previous).count();
	previous = now;
	frameMsTotal_ += frameMs;
	frameMsMax_ = (frameMs > frameMsMax_) ? frameMs : frameMsMax_;
	if (++frameSamples_ >= kSampleFrames)
	{
		lastAverageMs_ = frameMsTotal_ / frameSamples_;
		lastMaxMs_ = frameMsMax_;
		lastDrawn_ = KCE::GameObjectManager::GetInstance()->GetLastFrameDrawnCount();
		lastCulled_ = KCE::GameObjectManager::GetInstance()->GetLastFrameCulledCount();
		lastInstanced_ = KCE::GameObjectManager::GetInstance()->GetLastFrameInstancedCount();
		lastGroups_ = KCE::GameObjectManager::GetInstance()->GetLastFrameInstancedGroupCount();
		frameMsTotal_ = 0.0;
		frameMsMax_ = 0.0;
		frameSamples_ = 0;
	}

	if (!animate_)
	{
		return;
	}
	elapsed_ += static_cast<float>(frameMs) * 0.001f;

	if (path_ == Path::GameObject)
	{
		for (size_t i = 0; i < gameObjects_.size(); ++i)
		{
			KCE::Vector3 position;
			float rotationY = 0.0f;
			GetCrowdTransform(static_cast<int>(i), elapsed_, position, rotationY);
			gameObjects_[i]->SetPosition(position);
			gameObjects_[i]->SetRotation({ 0.0f, rotationY, 0.0f });
		}
	}
	else
	{
		for (size_t i = 0; i < entities_.size(); ++i)
		{
			auto& transform = registry_.GetComponent<KCE::ecs::TransformComponent>(entities_[i]);
			float rotationY = 0.0f;
			GetCrowdTransform(static_cast<int>(i), elapsed_, transform.localPosition_, rotationY);
			transform.localRotation_ = { 0.0f, rotationY, 0.0f };
			// ECS 経路にはヒエラルキーが無いので、ここでワールド行列まで作る
			transform.worldMatrix_ = KCE::MakeAffineMatrix(transform.localScale_, transform.localRotation_, transform.localPosition_);
		}
	}
}

void CrowdBenchScene::Draw3D()
{
	BaseScene::Draw3D();
	if (path_ == Path::GameObject)
	{
		KCE::GameObjectManager::GetInstance()->Draw3D(sceneManager_->GetCameraManager());
	}
}

void CrowdBenchScene::DrawGBuffer()
{
	BaseScene::DrawGBuffer();
	KCE::Camera* camera = sceneManager_->GetCameraManager()->GetActiveCamera();
	if (path_ == Path::GameObject)
	{
		KCE::GameObjectManager::GetInstance()->DrawGBuffer(sceneManager_->GetCameraManager());
	}
	else if (camera)
	{
		KCE::InstancedRenderSystem::DrawGBufferGrouped(registry_, ecsRenderers_, camera);
	}
}

void CrowdBenchScene::DrawShadow()
{
	BaseScene::DrawShadow();
	if (path_ == Path::GameObject)
	{
		KCE::GameObjectManager::GetInstance()->DrawShadow();
		return;
	}
	KCE::Camera* camera = sceneManager_->GetCameraManager()->GetActiveCamera();
	if (camera)
	{
		KCE::InstancedRenderSystem::DrawShadowGrouped(registry_, ecsRenderers_, camera, sceneManager_->GetShadowMapManager());
	}
}

void CrowdBenchScene::Draw2D()
{
}

void CrowdBenchScene::WriteRecord()
{
	std::ofstream log(kRecordPath, std::ios::app);
	if (!log)
	{
		return;
	}
	log << (path_ == Path::GameObject ? "GameObject" : "ECS")
		<< "\tcount=" << count_
		<< "\tavgMs=" << lastAverageMs_
		<< "\tmaxMs=" << lastMaxMs_
		<< "\tfps=" << (lastAverageMs_ > 0.0 ? 1000.0 / lastAverageMs_ : 0.0)
		<< "\tdrawn=" << lastDrawn_
		<< "\tculled=" << lastCulled_
		<< "\tinstanced=" << lastInstanced_
		<< "\tgroups=" << lastGroups_
		<< "\n";
}

void CrowdBenchScene::DrawImGui()
{
#ifdef USE_IMGUI
	ImGui::TextWrapped("敵をたくさん出して、描き方ごとの速さを比べる。視点はエンジンのデバッグカメラ（Scene の上で右ドラッグ + WASD）。");

	ImGui::SeparatorText("条件");
	ImGui::Text("体数");
	for (const int choice : kCountChoices)
	{
		ImGui::SameLine();
		if (ImGui::RadioButton(std::to_string(choice).c_str(), count_ == choice))
		{
			count_ = choice;
			rebuildRequest_ = true;
		}
	}
	ImGui::Text("描き方");
	ImGui::SameLine();
	if (ImGui::RadioButton("GameObject 経路", path_ == Path::GameObject))
	{
		path_ = Path::GameObject;
		rebuildRequest_ = true;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("ECS 経路", path_ == Path::Ecs))
	{
		path_ = Path::Ecs;
		rebuildRequest_ = true;
	}
	ImGui::Checkbox("動かす", &animate_);

	ImGui::SeparatorText("計測");
	ImGui::Text("体数: %d    道: %s", count_, path_ == Path::GameObject ? "GameObject" : "ECS");
	ImGui::Text("フレーム: 平均 %.3f ms / 最大 %.3f ms (%.1f FPS)",
		lastAverageMs_, lastMaxMs_, lastAverageMs_ > 0.0 ? 1000.0 / lastAverageMs_ : 0.0);
	ImGui::Text("GameObject: 描いた %u / 省いた %u", lastDrawn_, lastCulled_);
	ImGui::Text("まとめて描いた: %u    まとまり: %u", lastInstanced_, lastGroups_);
	ImGui::Text("GameObject の数: %zu    エンティティの数: %zu", gameObjects_.size(), entities_.size());
	ImGui::TextDisabled("%d フレームごとに締めて表示する。FPS 固定が効いていると差が出ないので注意。", kSampleFrames);
	if (ImGui::Button("計測を記録"))
	{
		WriteRecord();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("%s へ書き足す", kRecordPath);
#endif
}
