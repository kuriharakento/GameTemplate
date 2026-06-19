#pragma once
#include "engine/gameobject/component/base/IActionComponent.h"
#include "math/Vector3.h"
#include <vector>
#include <memory>
#include <random>
#include "application/GameObject/Combatable/character/enemy/base/Node/BehaviorTree/BehaviorTree.h"

class GameObject;

/**
 * @brief ナイフを持つ近接攻撃型敵のAI行動コンポーネント
 *
 * ビヘイビアツリーを使用して、パトロール、追跡、近接攻撃などの行動を管理する
 */
namespace GameObjectComponent
{
	class KnifeEnemyBehavior : public IActionComponent
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param target 追跡対象のゲームオブジェクト
		 * @param rightArm 右腕のゲームオブジェクト
		 * @param leftArm 左腕のゲームオブジェクト
		 * @param knife ナイフのゲームオブジェクト
		 */
		KnifeEnemyBehavior(::GameObject* target, ::GameObject* rightArm, ::GameObject* leftArm, ::GameObject* knife);
		~KnifeEnemyBehavior() override;
		void DrawImGui();

		/**
		 * @brief フレームごとの更新処理
		 * @param owner このコンポーネントを所有するゲームオブジェクト
		 */
		void Update(::GameObject* owner) override;

		/**
		 * @brief ターゲットを設定する
		 * @param target 追跡対象のゲームオブジェクト
		 */
		void SetTarget(::GameObject* target) { target_ = target; }

		/**
		 * @brief 移動速度を設定する
		 * @param speed 移動速度
		 */
		void SetMoveSpeed(float speed) { moveSpeed_ = speed; }

		/**
		 * @brief 攻撃範囲を設定する
		 * @param range 攻撃範囲
		 */
		void SetAttackRange(float range) { attackRange_ = range; }

		/**
		 * @brief 攻撃中かどうかを取得する
		 * @return 攻撃中ならtrue
		 */
		bool IsAttacking() const { return isAttacking_; }

	private:
		// 定数
		// デフォルト移動速度
		static constexpr float kDefaultMoveSpeed = 4.5f;
		// デフォルト攻撃範囲
		static constexpr float kDefaultAttackRange = 3.0f;
		// デフォルト検知範囲
		static constexpr float kDefaultDetectionRange = 25.0f;
		// デフォルトパトロール半径
		static constexpr float kDefaultPatrolRadius = 14.0f;
		// デフォルトパトロール速度係数
		static constexpr float kDefaultPatrolSpeed = 0.6f;
		// デフォルト攻撃持続時間
		static constexpr float kDefaultAttackDuration = 1.0f;
		// 攻撃ヒットタイミング（0.0〜1.0）
		static constexpr float kAttackHitTiming = 0.5f;
		// 攻撃間隔
		static constexpr float kDefaultAttackInterval = 1.8f;
		// パトロール到達判定距離
		static constexpr float kPatrolArrivalThreshold = 1.5f;
		// 1フレームの最大移動距離
		static constexpr float kMaxMoveDistancePerFrame = 0.25f;
		// パトロールポイント数
		static constexpr int kPatrolPointCount = 8;
		// 攻撃モーションの回転開始角度
		static constexpr float kAttackRotationStart = -1.2f;
		// 攻撃モーションの回転終了角度
		static constexpr float kAttackRotationEnd = 2.5f;
		// ナイフ攻撃時のX位置
		static constexpr float kKnifeAttackPosX = -4.3f;
		// ナイフ攻撃時のY位置
		static constexpr float kKnifeAttackPosY = -0.6f;
		// ナイフ攻撃時のZ位置
		static constexpr float kKnifeAttackPosZ = 0.1f;
		// ナイフ攻撃時のY回転
		static constexpr float kKnifeAttackRotY = -1.57f;
		// 攻撃時の腕のZ位置
		static constexpr float kArmAttackPosZ = 1.5f;

		// ビヘイビアツリーを構築
		void BuildBehaviorTree();

		// 待機行動
		void IdleAction(::GameObject* owner);
		// パトロール行動
		bool PatrolAction(::GameObject* owner);
		// 追跡行動
		bool ChaseAction(::GameObject* owner);
		// 攻撃行動
		bool AttackAction(::GameObject* owner);

		// ターゲットが視界内にいるか確認
		bool IsTargetVisible(::GameObject* owner);
		// 攻撃範囲内にいるか確認
		bool IsInAttackRange(::GameObject* owner);
		// パトロールポイントを初期化
		void InitializePatrolPoints(const Vector3& centerPoint, float radius);
		// 移動速度を制限
		float LimitMovementSpeed(float baseSpeed, float dt);

		// 攻撃モーションの更新
		void UpdateAttackMotion(::GameObject* owner, float deltaTime);

		// 追跡対象
		::GameObject* target_ = nullptr;
		// ナイフオブジェクト
		::GameObject* knife_ = nullptr;
		// 右腕オブジェクト
		::GameObject* rightArm_ = nullptr;
		// 左腕オブジェクト
		::GameObject* leftArm_ = nullptr;

		// 移動速度
		float moveSpeed_ = kDefaultMoveSpeed;
		// 攻撃範囲（近接は短め）
		float attackRange_ = kDefaultAttackRange;
		// 検知範囲
		float detectionRange_ = kDefaultDetectionRange;

		// パトロールポイントのリスト
		std::vector<Vector3> patrolPoints_;
		// 現在のパトロールインデックス
		int currentPatrolIndex_ = 0;
		// パトロール半径
		float patrolRadius_ = kDefaultPatrolRadius;
		// パトロール初期化フラグ
		bool patrolInitialized_ = false;
		// パトロール速度係数
		float patrolSpeed_ = kDefaultPatrolSpeed;

		// 攻撃中フラグ
		bool isAttacking_ = false;
		// 攻撃進行度（0.0〜1.0）
		float attackProgress_ = 0.0f;
		// 攻撃持続時間
		float attackDuration_ = kDefaultAttackDuration;
		// 攻撃判定のタイミング（0.0〜1.0）
		float attackHitTiming_ = kAttackHitTiming;
		// 今回の攻撃で既にヒット済みか
		bool hasHitTarget_ = false;

		// 右腕の初期回転値
		Vector3 rightArmInitialRotation_;
		// 右腕の初期位置
		Vector3 rightArmInitialPosition_;
		// 左腕の初期回転値
		Vector3 leftArmInitialRotation_;
		// ナイフの初期回転値
		Vector3 knifeInitialRotation_;
		// ナイフの初期位置
		Vector3 knifeInitialPosition_;

		// 攻撃クールダウン
		float attackCooldown_ = 0.0f;
		// 攻撃間隔
		float attackInterval_ = kDefaultAttackInterval;

		// 乱数生成器
		std::mt19937 rng_;

		// ビヘイビアツリー
		std::unique_ptr<::BehaviorTree> behaviorTree_;

		// キャッシュ用オーナー（デバッグUI描画用）
		::GameObject* lastOwner_ = nullptr;
	};
}