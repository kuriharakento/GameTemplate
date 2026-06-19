#include "EnemyManager.h"
#include "application/combo/ComboManager.h"
#include "math/MathUtils.h"
#include <audio/Audio.h>
#include "effects/particle/ParticleManager.h"
#include "externals/imgui/imgui.h"
#ifdef USE_IMGUI
#include "manager/editor/DebugUIManager.h"
#endif
#include "time/TimeManager.h"

// ECS Components
#include "engine/ecs/components/TransformComponent.h"
#include "application/ecs/components/EnemyStateComponent.h"
#include "application/ecs/components/StatusComponent.h"
#include "engine/ecs/components/EnemyAIComponent.h"
#include "engine/ecs/components/InstancedRenderComponent.h"
#include "engine/ecs/components/CollisionResponseComponent.h"
#include "engine/ecs/components/ColliderComponent.h"
#include "application/ecs/CollisionConfig.h"
#include "engine/ecs/components/TagComponent.h" // ecs::TagComponent
#include "engine/ecs/system/InstancedRenderSystem.h"
#include "engine/manager/scene/CameraManager.h"
#include "math/VectorColorCodes.h"
#include "engine/gameobject/component/collision/OBBColliderComponent.h"

// Engine
#include "graphics/3d/Object3dCommon.h"
#include "manager/graphics/ModelManager.h"
#include "application/effect/BulletTrailManager.h"
#include "application/gameObject/component/action/BulletComponent.h"
#include "application/gameObject/base/GameObjectTag.h"

// BT Nodes
#include "application/gameObject/combatable/character/enemy/base/Node/ActionNode.h"
#include "application/gameObject/combatable/character/enemy/base/Node/ConditionNode.h"
#include "application/gameObject/combatable/character/enemy/base/Node/SelectorNode.h"
#include "application/gameObject/combatable/character/enemy/base/Node/SequenceNode.h"
#include "application/gameObject/combatable/character/enemy/base/Node/BehaviorTree/BehaviorTree.h"

using namespace ecs;
using namespace GameObjectComponent;

namespace {
	// --- AI Actions and Bullet Spawning ---

	void AimAtTarget(TransformComponent& transform, const Vector3& targetPos) {
		Vector3 direction = targetPos - transform.localPosition_;
		direction.NormalizeSelf();
		float angle = atan2(direction.x, direction.z);
		transform.localRotation_ = Vector3(0, angle, 0);
	}

	void FireEnemyBullet(EnemyManager* manager, const Vector3& startPos, const Vector3& targetPosition, float speed = 50.0f) {
		auto bullet = std::make_unique<Bullet>(gameObjectTag::weapon::EnemyBullet);
		Vector3 direction = Vector3::Normalize(targetPosition - startPos);
		direction.y = 0.0f;
		float rotationY = atan2f(direction.x, direction.z);

		bullet->Initialize(manager->GetObject3dCommon(), manager->GetLightManager(), startPos);
		bullet->SetRotation({ 0.0f, rotationY, 0.0f });
		bullet->SetScale(Vector3(0.3f, 0.3f, 1.0f));

		uint32_t trailId = BulletTrailManager::GetInstance().RegisterBullet(bullet->GetTransform());
		bullet->SetTrailId(trailId);

		auto bulletComp = std::make_unique<BulletComponent>();
		bulletComp->Initialize(direction, speed, 2.0f); // lifetime 2.0
		bullet->AddComponent("Bullet", std::move(bulletComp));

		auto colliderComp = std::make_unique<OBBColliderComponent>(bullet.get());
		colliderComp->SetOnEnter([ptr = bullet.get()](GameObject* other) {
			if (other->GetTag() == gameObjectTag::character::Player || other->GetTag() == gameObjectTag::item::Obstacle) {
				ptr->SetAlive(false);
			}
		});
		bullet->AddComponent("OBBCollider", std::move(colliderComp));

		manager->AddEnemyBullet(std::move(bullet));
		Audio::GetInstance()->PlayWave("fire_se", false);
	}

	std::unique_ptr<BehaviorTree> BuildAssaultEnemyTree() {
		auto root = std::make_unique<SelectorNode>();

		// 0. スポーン待機
		root->AddChild(std::make_unique<ActionNode>("SpawnWarmup", [](Blackboard& bb) {
			float spawnTimer = bb.Get<float>("SpawnTimer");
			if (spawnTimer < 2.0f) return NodeStatus::Running;
			return NodeStatus::Failure;
		}));

		// 1. 戦闘：見えていて、攻撃範囲内
		auto combatSeq = std::make_unique<SequenceNode>();
		combatSeq->AddChild(std::make_unique<ConditionNode>([](Blackboard& bb) {
			return bb.Get<bool>("IsTargetVisible") && bb.Get<bool>("IsInAttackRange");
		}));
		// 戦闘中の行動（今はストレイフ等は省き、シンプルに射撃する）
		combatSeq->AddChild(std::make_unique<ActionNode>("Fire", [](Blackboard& bb) {
			EntityID owner = bb.Get<EntityID>("Owner");
			Registry* reg = bb.Get<Registry*>("Registry");
			EnemyManager* manager = bb.Get<EnemyManager*>("EnemyManager");
			auto& ai = reg->GetComponent<EnemyAIComponent>(owner);
			auto& transform = reg->GetComponent<TransformComponent>(owner);
			Vector3 targetPos = bb.Get<Vector3>("TargetPosition");

			AimAtTarget(transform, targetPos);

			if (ai.actionCooldown_ <= 0.0f) {
				FireEnemyBullet(manager, transform.localPosition_, targetPos);
				ai.actionCooldown_ = 0.2f; // Assault Fire Rate
			}
			return NodeStatus::Success;
		}));
		root->AddChild(std::move(combatSeq));

		// 2. 移動：見えていない、または範囲外なら近づく
		auto approachSeq = std::make_unique<SequenceNode>();
		approachSeq->AddChild(std::make_unique<ConditionNode>([](Blackboard& bb) {
			return bb.Get<EntityID>("Target") != kInvalidEntity;
		}));
		approachSeq->AddChild(std::make_unique<ActionNode>("Approach", [](Blackboard& bb) {
			EntityID owner = bb.Get<EntityID>("Owner");
			Registry* reg = bb.Get<Registry*>("Registry");
			auto& ai = reg->GetComponent<EnemyAIComponent>(owner);
			auto& transform = reg->GetComponent<TransformComponent>(owner);
			Vector3 targetPos = bb.Get<Vector3>("TargetPosition");

			Vector3 direction = targetPos - transform.localPosition_;
			float dist = direction.Length();

			float optimalDistance = (ai.attackRange_ + ai.minRange_) / 2.0f;
			auto& status = reg->GetComponent<ecs::StatusComponent>(owner);
			
			// [BNS-Fix] 瞬間移動防止のため deltaTime を制限
			float dt = TimeManager::GetInstance().GetGameContext().deltaTime;
			dt = (std::min)(dt, 0.033f);
			float moveFactor = dt * status.moveSpeed_.GetValue();

			if (dist > optimalDistance) {
				direction.NormalizeSelf();
				// [BNS-Fix] 移動量を残りの距離でクランプ
				float actualMove = (std::min)(moveFactor, dist - optimalDistance);
				transform.localPosition_ += direction * actualMove;
			}
			AimAtTarget(transform, targetPos);
			return NodeStatus::Running;
		}));
		root->AddChild(std::move(approachSeq));

		// 3. 待機
		root->AddChild(std::make_unique<ActionNode>("Idle", [](Blackboard& bb) { return NodeStatus::Running; }));

		return std::make_unique<BehaviorTree>(std::move(root));
	}

	std::unique_ptr<BehaviorTree> BuildPistolEnemyTree() {
		auto root = std::make_unique<SelectorNode>();
		auto combatSeq = std::make_unique<SequenceNode>();
		combatSeq->AddChild(std::make_unique<ConditionNode>([](Blackboard& bb) { return true; }));
		combatSeq->AddChild(std::make_unique<ActionNode>("PistolFire", [](Blackboard& bb) {
			EntityID owner = bb.Get<EntityID>("Owner");
			Registry* reg = bb.Get<Registry*>("Registry");
			EnemyManager* manager = bb.Get<EnemyManager*>("EnemyManager");
			auto& ai = reg->GetComponent<EnemyAIComponent>(owner);
			auto& transform = reg->GetComponent<TransformComponent>(owner);
			Vector3 targetPos = bb.Get<Vector3>("TargetPosition");

			AimAtTarget(transform, targetPos);

			if (ai.actionCooldown_ <= 0.0f) {
				FireEnemyBullet(manager, transform.localPosition_, targetPos, 40.0f);
				ai.actionCooldown_ = 1.0f; // Pistol Fire Rate
			}
			return NodeStatus::Success;
		}));
		root->AddChild(std::move(combatSeq));
		return std::make_unique<BehaviorTree>(std::move(root));
	}
}

void EnemyManager::Initialize(Registry* registry, SystemManager* systemManager, Object3dCommon* object3dCommon, SpriteCommon* spriteCommon, CameraManager* camera, LightManager* lightManager, ShadowMapManager* shadowMapManager, GameObject* target)
{
	registry_ = registry;
	systemManager_ = systemManager;
	object3dCommon_ = object3dCommon;
	spriteCommon_ = spriteCommon;
	camera_ = camera;
	lightManager_ = lightManager;
	shadowMapManager_ = shadowMapManager;
	target_ = target;

	emitRange_ = { { -10.0f, 1.0f, -10.0f }, { 10.0f, 1.0f, 10.0f } };

#ifdef USE_IMGUI
	DebugUIManager::GetInstance()->RegisterDebugUI(this, "Enemy Manager", [this]() { this->DrawImGui(); }, DebugUIArea::Inspector);
#endif
}

EnemyManager::~EnemyManager()
{
#ifdef USE_IMGUI
	if (DebugUIManager::HasInstance()) {
		DebugUIManager::GetInstance()->UnregisterDebugUI(this);
	}
#endif
}

void EnemyManager::DrawImGui()
{
#ifdef USE_IMGUI
	ImGui::Text("Active Entities: %d", registry_ ? registry_->GetActiveEntityCount() : 0);
	ImGui::Text("Enemy Bullets: %zu", enemyBullets_.size());
	
	if (ImGui::Button("Add Pistol Enemy (1)")) AddPistolEnemy(1);
	ImGui::SameLine();
	if (ImGui::Button("Add Pistol Enemy (100)")) AddPistolEnemy(100);

	if (ImGui::Button("Add Assault Enemy (1)")) AddAssaultEnemy(1);
	ImGui::SameLine();
	if (ImGui::Button("Add Assault Enemy (100)")) AddAssaultEnemy(100);

	if (ImGui::Button("Clear All Enemies")) Clear();

	ImGui::Separator();
	ImGui::Text("Behavior Trees:");
	if (registry_ && registry_->HasComponentArray<EnemyAIComponent>()) {
		auto view = registry_->View<EnemyAIComponent>();
		if (view) {
			for (uint32_t i = 0; i < view->GetSize(); ++i) {
				auto& ai = view->GetDataFromDenseIndex(i);
				EntityID entity = view->GetEntityFromDenseIndex(i);
				std::string label = "Enemy [Entity: " + std::to_string(entity) + "]";
				if (ImGui::TreeNode(label.c_str())) {
					if (ai.behaviorTree_) {
						ai.behaviorTree_->DrawImGui();
					} else {
						ImGui::Text("No BehaviorTree");
					}
					ImGui::TreePop();
				}
			}
		}
	}
#endif
}

void EnemyManager::Update()
{

	// Update bullets (Managed by GameObject)
	for (auto& bullet : enemyBullets_) {
		if (bullet->IsAlive()) bullet->Update();
	}
	for (auto it = enemyBullets_.begin(); it != enemyBullets_.end();) {
		if (!(*it)->IsAlive()) {
			BulletTrailManager::GetInstance().UnregisterBullet((*it)->GetTrailId());
			it = enemyBullets_.erase(it);
		} else {
			++it;
		}
	}

	// AI内でEnemyManagerポインタが必要なので、Blackboardに挿入する
	// EnemyBehaviorSystemにてBlackboard更新を行っているが、Managerポインタだけはここで各コンポーネントに刺しておくか、
	// 構築時にセットしておく（BuildTree後に固定するのでこの手立ては有効）
	if (registry_ && registry_->HasComponentArray<EnemyAIComponent>()) {
		auto view = registry_->View<EnemyAIComponent>();
		if (view) {
			for (uint32_t i = 0; i < view->GetSize(); ++i) {
			auto& ai = view->GetDataFromDenseIndex(i);
			if (ai.behaviorTree_) {
				ai.behaviorTree_->GetBlackboard().Set<EnemyManager*>("EnemyManager", this);
			}
			}
		}
	}

	// On All Defeated Event
	if (registry_->GetActiveEntityCount() == 0 && onAllEnemiesDefeatedCallback_)
	{
		onAllEnemiesDefeatedCallback_();
		onAllEnemiesDefeatedCallback_ = nullptr;
	}
}

void EnemyManager::DrawStandard3D(CameraManager* camera)
{
	for (auto& bullet : enemyBullets_) {
		if (bullet->IsAlive()) bullet->Draw3D(camera);
	}
}


void EnemyManager::AddPistolEnemy(uint32_t count)
{
	if (!registry_) return;
	for (uint32_t i = 0; i < count; ++i)
	{
		EntityID entity = registry_->CreateEntity();
		if (entity == kInvalidEntity) continue;

		Vector3 randomPos = MathUtils::RandomVector3(emitRange_.min_, emitRange_.max_);
		
		TransformComponent transform;
		transform.localPosition_ = randomPos;
		registry_->AddComponent<TransformComponent>(entity, transform);
		
		ecs::TagComponent enemyTag;
		enemyTag.type = ecs::TagComponent::Type::Enemy;
		registry_->AddComponent<ecs::TagComponent>(entity, enemyTag);

		EnemyStateComponent state;
		registry_->AddComponent<EnemyStateComponent>(entity, state);
		registry_->AddComponent<ecs::StatusComponent>(entity, ecs::StatusComponent{});
		AssignInstancedRenderComponent(entity, "enemy");

		EnemyAIComponent ai;
		ai.behaviorTree_ = BuildPistolEnemyTree();
		// Assuming Player is target, or wait for Scene to assign TargetEntity
		registry_->AddComponent<EnemyAIComponent>(entity, std::move(ai));
		AddCollisionComponents(entity);
	}
}

void EnemyManager::AddAssaultEnemy(uint32_t count)
{
	if (!registry_) return;
	for (uint32_t i = 0; i < count; ++i)
	{
		EntityID entity = registry_->CreateEntity();
		if (entity == kInvalidEntity) continue;

		Vector3 randomPos = MathUtils::RandomVector3(emitRange_.min_, emitRange_.max_);
		
		TransformComponent transform;
		transform.localPosition_ = randomPos;
		registry_->AddComponent<TransformComponent>(entity, transform);
		
		ecs::TagComponent enemyTag;
		enemyTag.type = ecs::TagComponent::Type::Enemy;
		registry_->AddComponent<ecs::TagComponent>(entity, enemyTag);

		EnemyStateComponent state;
		registry_->AddComponent<EnemyStateComponent>(entity, state);
		registry_->AddComponent<ecs::StatusComponent>(entity, ecs::StatusComponent{});
		AssignInstancedRenderComponent(entity, "enemy");

		EnemyAIComponent ai;
		ai.behaviorTree_ = BuildAssaultEnemyTree();
		registry_->AddComponent<EnemyAIComponent>(entity, std::move(ai));
		AddCollisionComponents(entity);
	}
}

void EnemyManager::AddShotgunEnemy(uint32_t count) { AddAssaultEnemy(count); }
void EnemyManager::AddKnifeEnemy(uint32_t count) { AddAssaultEnemy(count); }

void EnemyManager::SetEnemyData(const std::vector<GameObjectInfo>& data)
{
	enemyData_ = data;
	Clear();
	CreateAssaultEnemyFromData();
}

void EnemyManager::AddEnemiesFromGameObjectInfo(const std::vector<GameObjectInfo>& data)
{
	if (!registry_) return;
	for (size_t i = 0; i < data.size(); i++)
	{
		EntityID entity = registry_->CreateEntity();
		if (entity == kInvalidEntity) continue;

		TransformComponent transform;
		transform.localPosition_ = {data[i].transform.translate.x, data[i].transform.translate.y, data[i].transform.translate.z};
		registry_->AddComponent<TransformComponent>(entity, transform);

		ecs::TagComponent enemyTag;
		enemyTag.type = ecs::TagComponent::Type::Enemy;
		registry_->AddComponent<ecs::TagComponent>(entity, enemyTag);

		EnemyStateComponent state;
		registry_->AddComponent<EnemyStateComponent>(entity, state);
		registry_->AddComponent<ecs::StatusComponent>(entity, ecs::StatusComponent{});
		AssignInstancedRenderComponent(entity, "enemy");

		EnemyAIComponent ai;
		ai.behaviorTree_ = BuildAssaultEnemyTree();
		registry_->AddComponent<EnemyAIComponent>(entity, std::move(ai));
		AddCollisionComponents(entity);
	}
}

void EnemyManager::Clear()
{
	enemyBullets_.clear();
	// ECSのEntity群はSceneのRegistry初期化時か全クリ時などに消える
}

void EnemyManager::CreateAssaultEnemyFromData()
{
	AddEnemiesFromGameObjectInfo(enemyData_);
}

void EnemyManager::AssignInstancedRenderComponent(EntityID entity, const std::string& modelName)
{
	if (!registry_) return;
	InstancedRenderComponent render;
	render.modelName_ = modelName;
	render.useInstancing_ = true;
	render.isVisible_ = true;
	registry_->AddComponent<InstancedRenderComponent>(entity, render);
}

void EnemyManager::AddCollisionComponents(EntityID entity)
{
	if (!registry_) return;

	auto& transform = registry_->GetComponent<TransformComponent>(entity);

	// コライダー設定 (Sphere)
	ecs::ColliderComponent col;
	col.type_ = ColliderType::Sphere;
	col.sphere_.radius = 1.0f;
	col.useSubstep_ = true;
	col.previousPosition_ = transform.localPosition_;

	// フィルタリング
	col.layer = CollisionLayer::kEnemy;
	col.mask = CollisionLayer::kPlayer | CollisionLayer::kObstacle | CollisionLayer::kPlayerBullet;

	// 衝突応答
	col.onCollisionEnter = [reg = registry_, entity](const ecs::CollisionPartnerInfo& other) {
		if (reg->HasComponent<ecs::ColliderComponent>(other.entity)) {
			auto& otherCol = reg->GetComponent<ecs::ColliderComponent>(other.entity);
			
			// プレイヤーの弾に当たった時の処理（ダメージは弾側でも処理されるが、一応ここでもHPを減らすか、空にする）
			if (otherCol.layer & CollisionLayer::kPlayerBullet) {
				// ここでは何もしない。PlayerActionSystem または他の弾丸処理でHPを減らす。
			}
			// プレイヤーに当たったらダメージを与え、自分（敵）もダメージを受ける
			else if (otherCol.layer & CollisionLayer::kPlayer) {
				if (reg->HasComponent<ecs::StatusComponent>(other.entity)) {
					auto& status = reg->GetComponent<ecs::StatusComponent>(other.entity);
					float currentHp = status.hp_.GetBase();
					status.hp_.SetBase(currentHp - 10.0f); // プレイヤーへのダメージ
				}
				if (reg->HasComponent<ecs::StatusComponent>(entity)) {
					auto& status = reg->GetComponent<ecs::StatusComponent>(entity);
					status.hp_.SetBase(status.hp_.GetBase() - 50.0f); // 衝突による敵へのダメージ
				}
			}
		}
	};

	registry_->AddComponent<ecs::ColliderComponent>(entity, col);

	// レスポンス状態追跡用コンポーネント
	registry_->AddComponent<CollisionResponseComponent>(entity, {});
}
