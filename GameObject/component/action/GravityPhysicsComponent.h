#pragma once

#include "engine/gameobject/component/base/IActionComponent.h"
#include "math/Vector3.h"

/**
 * @brief 重力演算専用コンポーネント（Physics系）
 *
 * オブジェクトに重力を適用し、垂直方向の移動を制御する
 */
namespace GameObjectComponent
{
	class GravityPhysicsComponent : public IActionComponent
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param gravity 重力加速度（デフォルト: 9.8f）
		 */
		explicit GravityPhysicsComponent(float gravity = kDefaultGravity);

		/**
		 * @brief フレームごとの更新処理
		 *
		 * 垂直速度を更新し、Y=0より下には落とさない
		 * @param owner このコンポーネントを所有するゲームオブジェクト
		 */
		void Update(::GameObject* owner) override;

	private:
		// デフォルトの重力加速度
		static constexpr float kDefaultGravity = 9.8f;
		// 地面の高さ
		static constexpr float kGroundHeight = 0.0f;

		// 重力加速度
		float gravity_;
		// 現在の垂直速度
		float verticalVelocity_;
	};
}