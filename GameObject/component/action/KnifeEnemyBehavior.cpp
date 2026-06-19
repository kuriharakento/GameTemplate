#include "KnifeEnemyBehavior.h"
#include "engine/gameobject/base/GameObject.h"
#include "application/GameObject/Combatable/weapon/Knife.h"
#include "time/TimeManager.h"
#include <random>
#include <cmath>
#include <numbers>

#include "application/GameObject/Combatable/character/enemy/base/Node/ActionNode.h"
#include "application/GameObject/Combatable/character/enemy/base/Node/ConditionNode.h"
#include "application/GameObject/combatable/character/enemy/base/Node/NodeUtils.h"
#include "application/GameObject/Combatable/character/enemy/base/Node/SelectorNode.h"
#include "application/GameObject/Combatable/character/enemy/base/Node/SequenceNode.h"
#include "imgui/imgui.h"
#ifdef USE_IMGUI
#include "manager/editor/DebugUIManager.h"
#endif
#include "math/Easing.h"

namespace GameObjectComponent
{
	// コンストラクタ：各種オブジェクトの初期化とビヘイビアツリーの構築
	KnifeEnemyBehavior::KnifeEnemyBehavior(::GameObject* target, ::GameObject* rightArm, ::GameObject* leftArm, ::GameObject* knife)
		: target_(target), rightArm_(rightArm), leftArm_(leftArm), knife_(knife)
	{
		// 乱数生成器を初期化
		std::random_device rd;
		rng_ = std::mt19937(rd());

		// 初期回転値と位置を保存
		rightArmInitialRotation_ = rightArm_->GetRotation();
		rightArmInitialPosition_ = rightArm_->GetPosition();
		leftArmInitialRotation_ = leftArm_->GetRotation();
		knifeInitialRotation_ = knife_->GetRotation();
		knifeInitialPosition_ = knife_->GetPosition();

		// ビヘイビアツリーを構築
		BuildBehaviorTree();

	#ifdef USE_IMGUI
		DebugUIManager::GetInstance()->RegisterDebugUI(this, "KnifeBehavior", [this]() { this->DrawImGui(); }, DebugUIArea::Inspector);
	#endif
	}

	KnifeEnemyBehavior::~KnifeEnemyBehavior()
	{
	#ifdef USE_IMGUI
		if (DebugUIManager::HasInstance()) {
			DebugUIManager::GetInstance()->UnregisterDebugUI(this);
		}
	#endif
	}

	// フレームごとの更新処理
	void KnifeEnemyBehavior::DrawImGui()
	{
	#ifdef USE_IMGUI
		if (!lastOwner_)
		{
			ImGui::Text("KnifeBehavior: No active owner cache.");
			return;
		}
		ImGui::Text("Attacking: %s", isAttacking_ ? "true" : "false");
		ImGui::Text("Attack Progress: %.2f", attackProgress_);
		ImGui::Text("Attack Cooldown: %.2f", attackCooldown_);
		ImGui::Text("Target Visible: %s", IsTargetVisible(lastOwner_) ? "true" : "false");
		ImGui::Text("In Attack Range: %s", IsInAttackRange(lastOwner_) ? "true" : "false");

		// ビヘイビアツリーの表示
		if (ImGui::TreeNode("BehaviorTree"))
		{
			if (behaviorTree_)
				nodeUtils::DrawBTNodeImGui(behaviorTree_->GetRoot());
			ImGui::TreePop();
		}
	#endif
	}

	void KnifeEnemyBehavior::Update(::GameObject* owner)
	{
		lastOwner_ = owner;
		float deltaTime = TimeManager::GetInstance().GetGameContext().deltaTime;

		// 攻撃クールダウンを減少
		if (attackCooldown_ > 0)
		{
			attackCooldown_ -= deltaTime;
		}

		// 腕のオブジェクトを取得（未設定の場合）
		if (!rightArm_)
		{
			rightArm_ = owner->GetChild(gameObjectTag::character::KnifeEnemyRightArm);
		}
		if (!leftArm_)
		{
			leftArm_ = owner->GetChild(gameObjectTag::character::KnifeEnemyLeftArm);
		}

		// 攻撃モーションの更新
		if (isAttacking_)
		{
			UpdateAttackMotion(owner, deltaTime);
		}

		// Blackboardへ状態情報をセット
		auto& bb = behaviorTree_->GetBlackboard();
		bb.Set<::GameObject*>("Owner", owner);
		bb.Set<::GameObject*>("Target", target_);
		bb.Set<Vector3>("TargetPosition", target_ ? target_->GetPosition() : Vector3());
		bb.Set<bool>("IsTargetVisible", IsTargetVisible(owner));
		bb.Set<bool>("IsInAttackRange", IsInAttackRange(owner));
		bb.Set<float>("AttackCooldown", attackCooldown_);
		bb.Set<bool>("IsAttacking", isAttacking_);

		// ビヘイビアツリーを実行
		behaviorTree_->Tick();
	}

	// ビヘイビアツリーの構築
	void KnifeEnemyBehavior::BuildBehaviorTree()
	{
		auto root = std::make_unique<SelectorNode>();

		// 1. 近接攻撃（ターゲット発見＆攻撃範囲内＆攻撃可能状態）
		auto attackSeq = std::make_unique<SequenceNode>();
		attackSeq->AddChild(std::make_unique<ConditionNode>([this](Blackboard& bb) {
			return bb.Get<bool>("IsTargetVisible") &&
				bb.Get<bool>("IsInAttackRange") &&
				bb.Get<float>("AttackCooldown") <= 0.0f &&
				!bb.Get<bool>("IsAttacking");
															}));
		attackSeq->AddChild(std::make_unique<ActionNode>(
			"Attack",
			[this](Blackboard& bb) {
				auto owner = bb.Get<::GameObject*>("Owner");
				if (AttackAction(owner))
				{
					return NodeStatus::Success;
				}
				// 攻撃範囲外になった場合
				if (!IsInAttackRange(owner))
				{
					return NodeStatus::Failure;
				}
				return NodeStatus::Running;
			}));
		root->AddChild(std::move(attackSeq));

		// 2. 追従（発見済み＆攻撃中でない）
		auto chaseSeq = std::make_unique<SequenceNode>();
		chaseSeq->AddChild(std::make_unique<ConditionNode>([this](Blackboard& bb) {
			return bb.Get<bool>("IsTargetVisible") && !bb.Get<bool>("IsAttacking");
														   }));
		chaseSeq->AddChild(std::make_unique<ActionNode>(
			"Chase",
			[this](Blackboard& bb) {
				auto owner = bb.Get<::GameObject*>("Owner");
				if (ChaseAction(owner))
				{
					return NodeStatus::Success;
				}
				return NodeStatus::Running;
			}));
		root->AddChild(std::move(chaseSeq));

		// 3. パトロール（未発見かつ攻撃中でない）
		auto patrolSeq = std::make_unique<SequenceNode>();
		patrolSeq->AddChild(std::make_unique<ConditionNode>([this](Blackboard& bb) {
			return !bb.Get<bool>("IsTargetVisible") && !bb.Get<bool>("IsAttacking");
															}));
		patrolSeq->AddChild(std::make_unique<ActionNode>(
			"Patrol",
			[this](Blackboard& bb) {
				auto owner = bb.Get<::GameObject*>("Owner");
				if (PatrolAction(owner))
				{
					return NodeStatus::Success;
				}
				return NodeStatus::Running;
			}));
		root->AddChild(std::move(patrolSeq));

		// 4. 待機行動（他の条件に該当しない場合）
		root->AddChild(std::make_unique<ActionNode>(
			"Idle",
			[this](Blackboard& bb) {
				auto owner = bb.Get<::GameObject*>("Owner");
				IdleAction(owner);
				return NodeStatus::Running;
			}));

		behaviorTree_ = std::make_unique<BehaviorTree>(std::move(root));
	}

	// 待機行動（何もしない）
	void KnifeEnemyBehavior::IdleAction(::GameObject* owner)
	{
		// 何もしない
	}

	// パトロール行動
	bool KnifeEnemyBehavior::PatrolAction(::GameObject* owner)
	{
		// パトロールポイントが未初期化の場合は初期化
		if (patrolPoints_.empty())
		{
			InitializePatrolPoints(owner->GetPosition(), patrolRadius_);
		}

		// 目標のパトロールポイントを取得
		Vector3 targetPoint = patrolPoints_[currentPatrolIndex_];
		Vector3 dir = targetPoint - owner->GetPosition();
		float dist = dir.Length();

		// 目標地点に到達した場合、次のポイントへ
		if (dist < kPatrolArrivalThreshold)
		{
			currentPatrolIndex_ = (currentPatrolIndex_ + 1) % patrolPoints_.size();
			return true;
		}

		// 目標地点へ移動
		dir.NormalizeSelf();
		float moveDistance = LimitMovementSpeed(moveSpeed_ * patrolSpeed_, TimeManager::GetInstance().GetGameContext().deltaTime);
		owner->SetPosition(owner->GetPosition() + dir * moveDistance);

		// 移動方向を向く
		float angle = atan2(dir.x, dir.z);
		owner->SetRotation(Vector3(0, angle, 0));

		return false;
	}

	// 追跡行動
	bool KnifeEnemyBehavior::ChaseAction(::GameObject* owner)
	{
		// 目標消失時は終了
		if (!target_) return true;

		// ターゲットの位置を取得
		Vector3 targetPos = target_->GetPosition();
		Vector3 dir = targetPos - owner->GetPosition();
		float dist = dir.Length();

		// 攻撃レンジに入ったらSuccess
		if (dist < attackRange_)
		{
			return true;
		}

		// ターゲットへ移動
		dir.NormalizeSelf();
		float moveDistance = LimitMovementSpeed(moveSpeed_, TimeManager::GetInstance().GetGameContext().deltaTime);
		owner->SetPosition(owner->GetPosition() + dir * moveDistance);

		// 移動方向を向く
		float angle = atan2(dir.x, dir.z);
		owner->SetRotation(Vector3(0, angle, 0));

		return false;
	}

	// 攻撃行動
	bool KnifeEnemyBehavior::AttackAction(::GameObject* owner)
	{
		// 攻撃範囲外なら即Failureを返してChaseに遷移
		if (!IsInAttackRange(owner))
		{
			// 攻撃状態をリセット
			isAttacking_ = false;
			attackProgress_ = 0.0f;
			knife_->SetPosition(knifeInitialPosition_);
			knife_->SetRotation(knifeInitialRotation_);
			rightArm_->SetPosition(rightArmInitialPosition_);
			rightArm_->SetRotation(rightArmInitialRotation_);
			return false;
		}

		// 既に攻撃中の場合は攻撃の完了を待つ
		if (isAttacking_)
		{
			// 攻撃完了
			if (attackProgress_ >= 1.0f)
			{
				// 攻撃状態をリセット
				isAttacking_ = false;
				attackProgress_ = 0.0f;
				knife_->SetPosition(knifeInitialPosition_);
				knife_->SetRotation(knifeInitialRotation_);
				rightArm_->SetPosition(rightArmInitialPosition_);
				rightArm_->SetRotation(rightArmInitialRotation_);
				return true;
			}
			return false;
		}

		// クールダウン中なら攻撃不可
		if (attackCooldown_ > 0.0f)
		{
			return false;
		}

		// ターゲットの方向を向く
		if (target_)
		{
			Vector3 targetPos = target_->GetPosition();
			Vector3 direction = targetPos - owner->GetPosition();
			direction.y = 0.0f;
			direction.NormalizeSelf();

			float angle = atan2(direction.x, direction.z);
			owner->SetRotation(Vector3(0, angle, 0));
		}

		// 攻撃開始
		isAttacking_ = true;
		attackProgress_ = 0.0f;
		hasHitTarget_ = false;
		attackCooldown_ = attackInterval_;

		return false;
	}

	// ターゲットが視界内にいるか確認
	bool KnifeEnemyBehavior::IsTargetVisible(::GameObject* owner)
	{
		if (!target_) return false;

		// ターゲットまでの距離を計算
		Vector3 targetPos = target_->GetPosition();
		Vector3 direction = targetPos - owner->GetPosition();
		float distance = direction.Length();

		// 検知範囲内にいるかどうかを返す
		return (distance <= detectionRange_);
	}

	// 攻撃範囲内にいるか確認
	bool KnifeEnemyBehavior::IsInAttackRange(::GameObject* owner)
	{
		if (!target_) return false;

		// ターゲットまでの距離を計算
		Vector3 targetPos = target_->GetPosition();
		Vector3 direction = targetPos - owner->GetPosition();
		float distance = direction.Length();

		// 攻撃範囲内にいるかどうかを返す
		return (distance <= attackRange_);
	}

	// パトロールポイントを初期化
	void KnifeEnemyBehavior::InitializePatrolPoints(const Vector3& centerPoint, float radius)
	{
		patrolPoints_.clear();

		// 円周上に等間隔でパトロールポイントを生成
		for (int i = 0; i < kPatrolPointCount; i++)
		{
			float angle = (i * 2.0f * std::numbers::pi_v<float>) / kPatrolPointCount;
			float x = centerPoint.x + radius * std::cos(angle);
			float z = centerPoint.z + radius * std::sin(angle);
			patrolPoints_.push_back(Vector3(x, centerPoint.y, z));
		}

		// ランダムな開始位置を設定
		std::random_device rd;
		rng_ = std::mt19937(rd());
		currentPatrolIndex_ = std::uniform_int_distribution<int>(0, kPatrolPointCount - 1)(rng_);
		patrolInitialized_ = true;
	}

	// 移動速度を制限
	float KnifeEnemyBehavior::LimitMovementSpeed(float baseSpeed, float dt)
	{
		// 1フレームあたりの移動距離に上限を設定
		return std::min(baseSpeed * dt, kMaxMoveDistancePerFrame);
	}

	// 攻撃モーションの更新
	void KnifeEnemyBehavior::UpdateAttackMotion(::GameObject* owner, float deltaTime)
	{
		if (!isAttacking_) return;

		// 攻撃中に攻撃範囲外になった場合は即中断
		if (!IsInAttackRange(owner) || attackProgress_ >= 1.0f)
		{
			// 攻撃状態をリセット
			isAttacking_ = false;
			attackProgress_ = 0.0f;
			knife_->SetPosition(knifeInitialPosition_);
			knife_->SetRotation(knifeInitialRotation_);
			rightArm_->SetPosition(rightArmInitialPosition_);
			rightArm_->SetRotation(rightArmInitialRotation_);
			return;
		}

		// 攻撃進行度を更新
		attackProgress_ += deltaTime / attackDuration_;
		float t = std::clamp(attackProgress_, 0.0f, 1.0f);

		// イージングで強弱をつける
		float easeT = EaseInOutExpo(t);

		// 左から右への回転アニメーション
		float y = kAttackRotationStart + (kAttackRotationEnd - kAttackRotationStart) * easeT;

		// 攻撃時の腕の回転を計算
		Vector3 attackRotation;
		attackRotation.x = -std::numbers::pi_v<float> / 2.0;
		attackRotation.y = y;
		attackRotation.z = 0.0;

		// ナイフと腕の位置・回転を更新
		knife_->SetPosition({ kKnifeAttackPosX, kKnifeAttackPosY, kKnifeAttackPosZ });
		knife_->SetRotation({ 0.0f, kKnifeAttackRotY, 0.0f });
		Vector3 rightArmPos = rightArmInitialPosition_;
		rightArmPos.z = kArmAttackPosZ;
		rightArm_->SetPosition(rightArmPos);
		rightArm_->SetRotation(attackRotation);
	}
}