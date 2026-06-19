#pragma once
#include "application/gameObject/combatable/base/StatusSystem.h"
#include "engine/gameobject/component/base/IActionComponent.h"
#include "jsonEditor/JsonEditableBase.h"

/**
 * @brief ゲームオブジェクトのステータスを管理するコンポーネント
 *
 * HP、攻撃力、移動速度などのステータス値を管理し、更新処理を行う
 */
namespace GameObjectComponent
{
	class StatusComponent : public IActionComponent, public JsonEditableBase
	{
	public:
		StatusComponent();
		~StatusComponent() override;
		void Update(::GameObject* owner) override;
		void DrawImGui();

		// 現在のHP
		StatusValue hp{ kDefaultHp };
		// 最大HP
		StatusValue maxHp{ kDefaultMaxHp };
		// 攻撃力
		StatusValue attackPower{ kDefaultAttackPower };
		// 移動速度
		StatusValue moveSpeed{ kDefaultMoveSpeed };
		// 射撃レート倍率
		StatusValue fireRateMultiplier{ kDefaultFireRateMultiplier };
		// 生存フラグ
		bool isAlive = true;

	private:
		// デフォルトHP
		static constexpr float kDefaultHp = 100.0f;
		// デフォルト最大HP
		static constexpr float kDefaultMaxHp = 100.0f;
		// デフォルト攻撃力
		static constexpr float kDefaultAttackPower = 10.0f;
		// デフォルト移動速度
		static constexpr float kDefaultMoveSpeed = 9.0f;
		// デフォルト射撃レート倍率
		static constexpr float kDefaultFireRateMultiplier = 1.0f;
		// 死亡判定のHP閾値
		static constexpr float kDeathThreshold = 0.0f;

		// キャッシュ用オーナー（デバッグUI描画用）
		::GameObject* lastOwner_ = nullptr;
	};
}