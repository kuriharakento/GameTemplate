#include "DebugScene.h"
#include "base/Logger.h"
#include "gameobject/base/GameObject.h"
#include "gameobject/component/base/Behaviour.h"
#include "gameobject/component/collision/AABBCollider.h"
#include "gameobject/component/collision/OBBCollider.h"
#include "gameobject/component/collision/RayCollider.h"
#include "gameobject/component/collision/SphereCollider.h"
#include "gameobject/manager/GameObjectManager.h"
#include "manager/graphics/LineManager.h"
#include "manager/scene/CameraManager.h"
#include "math/VectorColorCodes.h"
#include "scene/factory/SceneFactory.h"
#include "scene/manager/SceneManager.h"
#include "time/TimeManager.h"
#include "imgui/imgui.h"

REGISTER_SCENE(DebugScene);

namespace
{
constexpr uint32_t kMovingLayer = 1u << 0;
constexpr uint32_t kRayTargetLayer = 1u << 1;
constexpr float kBulletTravelDistance = 30.0f;
constexpr char kDebugPrefabPath[] = "debug_multi_collider.json";
constexpr size_t kDebugPrefabColliderCount = 2;

class FastBullet final : public KCE::GameObjectComponent::Behaviour
{
public:
    void Update() override
    {
		if (moved_) return;
        auto* owner = GetOwner();
		owner->SetPosition(owner->GetPosition() + KCE::Vector3{ 0.0f, 0.0f, kBulletTravelDistance });
		moved_ = true;
    }
private:
	bool moved_ = false;
};
class CollisionCounter final : public KCE::GameObjectComponent::Behaviour
{
public:
	CollisionCounter(int* colliderCount, int* objectCount, int* colliderExitCount = nullptr)
		: colliderCount_(colliderCount), objectCount_(objectCount), colliderExitCount_(colliderExitCount) {}
	void OnCollisionEnter(const KCE::GameObjectComponent::CollisionInfo&) override { ++*colliderCount_; }
	void OnCollisionExit(const KCE::GameObjectComponent::CollisionInfo&) override
	{
		if (colliderExitCount_) ++*colliderExitCount_;
	}
	void OnObjectCollisionEnter(const KCE::GameObjectComponent::CollisionInfo&) override { ++*objectCount_; }
private:
	// DebugScene が所有し、このコンポーネントより後まで生きる。
	int* colliderCount_ = nullptr;
	int* objectCount_ = nullptr;
	int* colliderExitCount_ = nullptr;
};
constexpr int kExitSourceMoveFrame = 40;
constexpr int kExitCheckFrame = 42;
constexpr float kExitSourceAwayHeight = 50.0f;

class ExitReaction final : public KCE::GameObjectComponent::Behaviour
{
public:
	ExitReaction(KCE::GameObjectComponent::Collider* disableTarget, KCE::GameObject* removeTarget, int* count)
		: disableTarget_(disableTarget), removeTarget_(removeTarget), count_(count) {}
	void OnCollisionExit(const KCE::GameObjectComponent::CollisionInfo&) override
	{
		// 1回だけ。触れている最中の別の判定を、無効化とその場の削除の両方で外す
		if (done_) return;
		done_ = true;
		++*count_;
		disableTarget_->SetEnabled(false);
		// AddComponent<T> は Factory の正規名で登録するので、同じ名前で外す
		removeTarget_->RemoveComponent(KCE::GameObjectComponent::ComponentFactory::GetInstance()->GetTypeName(typeid(KCE::GameObjectComponent::AABBCollider)));
	}
private:
	// どれも DebugScene が所有し、このコンポーネントより長く生きる。
	KCE::GameObjectComponent::Collider* disableTarget_ = nullptr;
	KCE::GameObject* removeTarget_ = nullptr;
	int* count_ = nullptr;
	bool done_ = false;
};
class LifecycleProbe final : public KCE::GameObjectComponent::Behaviour
{
public:
	LifecycleProbe(int* awake, int* enable, int* start, int* update, int* lateUpdate, int* disable, int* destroy)
		: awake_(awake), enable_(enable), start_(start), update_(update), lateUpdate_(lateUpdate), disable_(disable), destroy_(destroy) {}
	void Awake() override { ++*awake_; }
	void OnEnable() override { ++*enable_; }
	void Start() override { ++*start_; }
	void Update() override { ++*update_; }
	void LateUpdate() override { ++*lateUpdate_; }
	void OnDisable() override { ++*disable_; }
	void OnDestroy() override { ++*destroy_; }
private:
	// 全カウンターは DebugScene が所有し、このコンポーネントより長く生きる。
	int* awake_;
	int* enable_;
	int* start_;
	int* update_;
	int* lateUpdate_;
	int* disable_;
	int* destroy_;
};
}

KCE::GameObject* DebugScene::CreateObject(const char* name, const KCE::Vector3& position, const KCE::Vector3& scale)
{
    auto object = std::make_unique<KCE::GameObject>();
    object->SetName(name);
    object->Initialize(sceneManager_->GetObject3dCommon(), sceneManager_->GetLightManager());
    object->SetPosition(position);
    object->SetScale(scale);
    object->UpdateWorldMatrix();
    auto* result = object.get();
    KCE::GameObjectManager::GetInstance()->Register(result);
    objects_.push_back(std::move(object));
    return result;
}

void DebugScene::Initialize()
{
    KCE::CollisionManager::GetInstance()->Initialize();
    sceneManager_->GetCameraManager()->GetActiveCamera()->SetTranslate({ 0.0f, 12.0f, -32.0f });
    sceneManager_->GetCameraManager()->GetActiveCamera()->SetRotate({ 0.25f, 0.0f, 0.0f });
	lifecycleObject_ = CreateObject("LifecycleProbe", { 0.0f, -6.0f, 0.0f }, { 0.5f, 0.5f, 0.5f });
	auto probe = std::make_unique<LifecycleProbe>(&awakeCount_, &enableCount_, &startCount_, &updateCount_, &lateUpdateCount_, &disableCount_, &destroyCount_);
	lifecycleProbe_ = lifecycleObject_->AddComponent(std::move(probe), "LifecycleProbe");

    auto* wall = CreateObject("ThinWall", { 0.0f, 0.0f, 0.0f }, { 4.0f, 4.0f, 0.2f });
    auto* wallCollider = wall->AddComponent<KCE::GameObjectComponent::AABBCollider>();
    wallCollider->SetCollisionLayer(kMovingLayer);

    auto* bullet = CreateObject("FastBullet", { 0.0f, 0.0f, -12.0f }, { 0.25f, 0.25f, 0.25f });
    bullet->AddComponent<FastBullet>();
    bullet->AddComponent<CollisionCounter>(&bulletHitCount_, &bulletHitCount_);
    auto* bulletCollider = bullet->AddComponent<KCE::GameObjectComponent::SphereCollider>();
    bulletCollider->SetSphere({ {}, 0.25f });
    bulletCollider->SetUseSubstep(true);
    bulletCollider->SetCollisionLayer(kMovingLayer);

    auto* body = CreateObject("MultiCollider", { 9.0f, 0.0f, 0.0f }, { 1.0f, 2.0f, 1.0f });
	body->AddComponent<CollisionCounter>(&colliderEnterCount_, &objectEnterCount_, &colliderExitCount_);
	bodyCollider_ = body->AddComponent<KCE::GameObjectComponent::OBBCollider>();
	bodyCollider_->SetCollisionLayer(kMovingLayer);
    auto* feetCollider = body->AddComponent<KCE::GameObjectComponent::SphereCollider>();
    feetCollider->SetSphere({ {}, 1.0f });
    feetCollider->SetCenter({ 0.0f, -1.5f, 0.0f });
    feetCollider->SetCollisionLayer(kMovingLayer);
    auto* contact = CreateObject("MultiColliderTarget", { 9.0f, -1.0f, 0.0f }, { 1.5f, 1.5f, 1.5f });
    contact->AddComponent<KCE::GameObjectComponent::AABBCollider>()->SetCollisionLayer(kMovingLayer);
    auto* spawnPathObstacle = CreateObject("SpawnPathObstacle", { 10.0f, 1.0f, 2.5f }, { 1.0f, 1.0f, 1.0f });
    spawnPathObstacle->AddComponent<KCE::GameObjectComponent::AABBCollider>()->SetCollisionLayer(kMovingLayer);

	// 離れた通知の中で別の判定を外す確認。ExitSource が離れた瞬間に、VictimA を無効化し、VictimB の判定を消す
	auto* victimA = CreateObject("ExitVictimA", { -24.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	auto* victimACollider = victimA->AddComponent<KCE::GameObjectComponent::AABBCollider>();
	victimACollider->SetCollisionLayer(kMovingLayer);
	auto* victimAPartner = CreateObject("ExitVictimAPartner", { -24.0f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	victimAPartner->AddComponent<CollisionCounter>(&exitVictimEnterCount_, &exitVictimEnterCount_, &victimExitCount_);
	victimAPartner->AddComponent<KCE::GameObjectComponent::AABBCollider>()->SetCollisionLayer(kMovingLayer);
	auto* victimB = CreateObject("ExitVictimB", { -28.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	victimB->AddComponent<KCE::GameObjectComponent::AABBCollider>()->SetCollisionLayer(kMovingLayer);
	auto* victimBPartner = CreateObject("ExitVictimBPartner", { -28.0f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	victimBPartner->AddComponent<CollisionCounter>(&exitVictimEnterCount_, &exitVictimEnterCount_, &victimExitCount_);
	victimBPartner->AddComponent<KCE::GameObjectComponent::AABBCollider>()->SetCollisionLayer(kMovingLayer);
	exitSource_ = CreateObject("ExitSource", { -20.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	exitSource_->AddComponent<ExitReaction>(victimACollider, victimB, &exitReactionCount_);
	exitSource_->AddComponent<KCE::GameObjectComponent::SphereCollider>()->SetCollisionLayer(kMovingLayer);
	auto* exitAnchor = CreateObject("ExitAnchor", { -20.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	exitAnchor->AddComponent<KCE::GameObjectComponent::AABBCollider>()->SetCollisionLayer(kMovingLayer);

    const KCE::Vector3 targetPositions[] = { { -8.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 10.0f }, { 8.0f, 0.0f, 10.0f } };
    auto* box = CreateObject("RayAABB", targetPositions[0], { 1.0f, 1.0f, 1.0f });
    box->AddComponent<KCE::GameObjectComponent::AABBCollider>()->SetCollisionLayer(kRayTargetLayer);
    auto* sphere = CreateObject("RaySphere", targetPositions[1], { 1.0f, 1.0f, 1.0f });
    sphere->AddComponent<KCE::GameObjectComponent::SphereCollider>()->SetCollisionLayer(kRayTargetLayer);
    auto* obb = CreateObject("RayOBB", targetPositions[2], { 1.0f, 1.0f, 1.0f });
    obb->SetRotation({ 0.0f, 0.5f, 0.0f });
    obb->AddComponent<KCE::GameObjectComponent::OBBCollider>()->SetCollisionLayer(kRayTargetLayer);
    for (const auto& position : targetPositions)
    {
        auto* rayObject = CreateObject("Ray", { position.x, 0.0f, -4.0f }, { 0.1f, 0.1f, 0.1f });
        auto* ray = rayObject->AddComponent<KCE::GameObjectComponent::RayCollider>();
        rayObject->AddComponent<CollisionCounter>(&rayEnterCount_, &rayEnterCount_);
        ray->SetLength(20.0f);
        ray->SetCollisionLayer(kMovingLayer);
        ray->SetCollisionMask(kRayTargetLayer);
    }

	// 体と足元の2判定を保存し、同じプレハブから独立した2体を作る
	auto* prefabSource = CreateObject("PrefabSource", { 0.0f, 100.0f, 0.0f }, { 1.0f, 2.0f, 1.0f });
	prefabSource->AddComponent<KCE::GameObjectComponent::OBBCollider>()->SetCollisionLayer(kMovingLayer);
	auto* prefabFeet = prefabSource->AddComponent<KCE::GameObjectComponent::SphereCollider>();
	prefabFeet->SetSphere({ {}, 1.0f });
	prefabFeet->SetCenter({ 0.0f, -1.5f, 0.0f });
	prefabFeet->SetCollisionLayer(kMovingLayer);
	prefabSaved_ = prefabSource->SavePrefab(kDebugPrefabPath);
	if (prefabSaved_)
	{
		KCE::Transform transformA;
		transformA.translate = { 14.0f, 0.0f, 0.0f };
		KCE::Transform transformB;
		transformB.translate = { 18.0f, 0.0f, 0.0f };
		prefabInstanceA_ = KCE::GameObjectManager::GetInstance()->Instantiate(kDebugPrefabPath, transformA);
		prefabInstanceB_ = KCE::GameObjectManager::GetInstance()->Instantiate(kDebugPrefabPath, transformB);
	}
	if (prefabInstanceA_ && prefabInstanceB_)
	{
		prefabInstanceA_->AddComponent<CollisionCounter>(&prefabEnterCountA_, &prefabEnterCountA_);
		prefabInstanceB_->AddComponent<CollisionCounter>(&prefabEnterCountB_, &prefabEnterCountB_);
		prefabComponentsRestored_ = prefabInstanceA_->GetComponents().size() >= kDebugPrefabColliderCount &&
			prefabInstanceB_->GetComponents().size() >= kDebugPrefabColliderCount;
		prefabGuidsDiffer_ = prefabInstanceA_->GetGuid() != prefabInstanceB_->GetGuid();
		auto* targetA = CreateObject("PrefabTargetA", { 14.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
		targetA->AddComponent<KCE::GameObjectComponent::AABBCollider>()->SetCollisionLayer(kMovingLayer);
		auto* targetB = CreateObject("PrefabTargetB", { 18.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
		targetB->AddComponent<KCE::GameObjectComponent::AABBCollider>()->SetCollisionLayer(kMovingLayer);
	}
}

void DebugScene::CommonUpdate()
{
    ++frameCount_;
	if (frameCount_ == 10 && bodyCollider_)
	{
		bodyCollider_->SetEnabled(false);
		KCE::Logger::Log(
			"DebugScene: disabled body collider; colliderExit=" + std::to_string(colliderExitCount_) +
			" objectEnter=" + std::to_string(objectEnterCount_));
	}
	if (frameCount_ == 20 && lifecycleProbe_) lifecycleProbe_->SetEnabled(false);
	if (frameCount_ == 30 && lifecycleProbe_) lifecycleProbe_->SetEnabled(true);
	if (frameCount_ == 35 && lifecycleObject_)
	{
		lifecycleObject_->RemoveComponent("LifecycleProbe");
		lifecycleProbe_ = nullptr;
	}
	if (frameCount_ == kExitSourceMoveFrame && exitSource_)
	{
		exitSource_->SetPosition({ -20.0f, kExitSourceAwayHeight, 0.0f });
	}
	if (frameCount_ == kExitCheckFrame)
	{
		KCE::Logger::Log(
			"DebugScene: exit reaction=" + std::to_string(exitReactionCount_) +
			" victimExit=" + std::to_string(victimExitCount_));
	}
    if (!spawnedBullet_ && frameCount_ >= 30)
    {
        auto* bullet = CreateObject("SpawnedBullet", { 20.0f, 1.0f, 5.0f }, { 0.25f, 0.25f, 0.25f });
        bullet->AddComponent<CollisionCounter>(&spawnedBulletFalseHitCount_, &spawnedBulletFalseHitCount_);
        auto* collider = bullet->AddComponent<KCE::GameObjectComponent::SphereCollider>();
        collider->SetSphere({ {}, 0.25f });
        collider->SetUseSubstep(true);
        collider->SetCollisionLayer(kMovingLayer);
        spawnedBullet_ = true;
    }
    auto* collisions = KCE::CollisionManager::GetInstance();
    collisions->UpdatePreviousPositions();
    KCE::GameObjectManager::GetInstance()->Update();
    collisions->CheckCollisions();
    KCE::Ray ray{ { 0.0f, 0.0f, -4.0f }, { 0.0f, 0.0f, 1.0f }, 20.0f };
	hasRaycastHit_ = collisions->Raycast(ray, kRayTargetLayer, raycastHit_);
}
void DebugScene::Draw3D()
{
    KCE::GameObjectManager::GetInstance()->Draw3D(sceneManager_->GetCameraManager());
    KCE::LineManager::GetInstance()->DrawGrid(40.0f, 2.0f, KCE::VectorColorCodes::White);
}
void DebugScene::Draw2D() {}
void DebugScene::DrawImGui()
{
    ImGui::Begin("Component Lifecycle Check");
    ImGui::Text("Collider Enter: %d", colliderEnterCount_);
	ImGui::Text("Collider Exit after disable: %d", colliderExitCount_);
	ImGui::Text("Active collision pairs: %zu", KCE::CollisionManager::GetInstance()->GetActiveCollisionCount());
    ImGui::Text("Object Enter: %d", objectEnterCount_);
    ImGui::Text("Fast bullet hit: %d", bulletHitCount_);
	ImGui::Text("Exit reaction / victim exit: %d / %d", exitReactionCount_, victimExitCount_);
    ImGui::Text("Ray collider enter: %d", rayEnterCount_);
    ImGui::Text("Spawned bullet false hit: %d", spawnedBulletFalseHitCount_);
	ImGui::Text("Raycast: %s", hasRaycastHit_ ? raycastHit_.object->GetName().c_str() : "None");
    ImGui::Text("Raycast distance: %.3f", raycastHit_.distance);
	ImGui::Text("Lifecycle A/E/S/U/L/D/X: %d/%d/%d/%d/%d/%d/%d", awakeCount_, enableCount_, startCount_, updateCount_, lateUpdateCount_, disableCount_, destroyCount_);
	ImGui::SeparatorText("Prefab Check");
	ImGui::Text("Saved / components / GUIDs differ: %s / %s / %s", prefabSaved_ ? "OK" : "NG",
		prefabComponentsRestored_ ? "OK" : "NG", prefabGuidsDiffer_ ? "OK" : "NG");
	ImGui::Text("Instance collision enter A / B: %d / %d", prefabEnterCountA_, prefabEnterCountB_);
    ImGui::End();
}
void DebugScene::OnFinalize()
{
	if (prefabInstanceA_) KCE::GameObjectManager::GetInstance()->Unregister(prefabInstanceA_);
	if (prefabInstanceB_) KCE::GameObjectManager::GetInstance()->Unregister(prefabInstanceB_);
	prefabInstanceA_ = nullptr;
	prefabInstanceB_ = nullptr;
    objects_.clear();
}
