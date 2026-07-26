#pragma once
#include "engine/gameobject/component/base/IActionComponent.h"
#include "jsonEditor/JsonEditableBase.h"
#include "math/Vector3.h"

namespace GameObjectComponent
{
	/**
	 * @brief キャラクターの物理挙動（移動速度と外部物理速度を分離して管理）を制御するコンポーネント
	 */
	class PhysicsComponent : public IActionComponent, public JsonEditableBase
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param owner このコンポーネントを所有するGameObject
		 */
		PhysicsComponent(::GameObject* owner);

		/**
		 * @brief デストラクタ
		 */
		~PhysicsComponent() override;

		/**
		 * @brief 毎フレームの更新処理
		 * @param owner このコンポーネントを所有するGameObject
		 */
		void Update(::GameObject* owner) override;

		/**
		 * @brief 瞬間的な外力（被弾の衝撃や反射時の反動など）を加える
		 * @param force 加える力ベクトル
		 */
		void AddExternalForce(const Vector3& force);

		/**
		 * @brief 継続的な外力（風など）を設定する
		 * @param force 設定する力ベクトル
		 */
		void SetConstantExternalForce(const Vector3& force) { constantExternalForce_ = force; }

		/**
		 * @brief キャラクター自身の意志による移動速度を設定する（プレイヤー入力などから直接上書き）
		 * @param velocity 設定する移動速度ベクトル
		 */
		void SetMovementVelocity(const Vector3& velocity) { movementVelocity_ = velocity; }

	public: // ゲッター・セッター
		// 最終的な合算速度を取得
		Vector3 GetVelocity() const { return movementVelocity_ + externalVelocity_; }

		// 互換用。外部物理速度を上書きします
		void SetVelocity(const Vector3& velocity) { externalVelocity_ = velocity; }

		const Vector3& GetMovementVelocity() const { return movementVelocity_; }
		
		const Vector3& GetExternalVelocity() const { return externalVelocity_; }
		void SetExternalVelocity(const Vector3& velocity) { externalVelocity_ = velocity; }

		const Vector3& GetExternalForce() const { return externalForce_; }
		const Vector3& GetConstantExternalForce() const { return constantExternalForce_; }

		float GetMass() const { return mass_; }
		void SetMass(float mass) { mass_ = mass; }

		float GetExternalFriction() const { return externalFriction_; }
		void SetExternalFriction(float friction) { externalFriction_ = friction; }

		float GetExternalAirResistance() const { return externalAirResistance_; }
		void SetExternalAirResistance(float resistance) { externalAirResistance_ = resistance; }

		bool GetUseGravity() const { return useGravity_; }
		void SetUseGravity(bool use) { useGravity_ = use; }

		bool IsGrounded() const { return isGrounded_; }
		void SetGrounded(bool grounded) { isGrounded_ = grounded; }

		float GetGravity() const { return gravity_; }
		void SetGravity(float gravity) { gravity_ = gravity; }

	private:
		// キャラクター自身の移動速度 (プレイヤー入力などで制御され、基本的には摩擦等の影響を受けない)
		Vector3 movementVelocity_ = { 0.0f, 0.0f, 0.0f };
		// 外部から加わった物理速度 (ノックバック、重力、風などによる速度)
		Vector3 externalVelocity_ = { 0.0f, 0.0f, 0.0f };

		// 瞬間的な外力 (Updateで消費されリセットされる)
		Vector3 externalForce_ = { 0.0f, 0.0f, 0.0f };
		// 継続的な外力 (風など)
		Vector3 constantExternalForce_ = { 0.0f, 0.0f, 0.0f };

		// 質量
		float mass_ = 1.0f;
		// 外部物理速度に対する地上摩擦係数 (1フレームあたりの速度残存率 0.0 ~ 1.0)
		float externalFriction_ = 0.8f;
		// 外部物理速度に対する空中抵抗係数 (1フレームあたりの速度残存率 0.0 ~ 1.0)
		float externalAirResistance_ = 0.98f;

		// 重力を適用するか
		bool useGravity_ = true;
		// 接地しているか
		bool isGrounded_ = false;
		// 重力加速度 (m/s^2)
		float gravity_ = 9.8f;
	};
}
