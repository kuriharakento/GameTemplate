#pragma once
#include "engine/gameobject/component/base/IActionComponent.h"
#include "jsonEditor/JsonEditableBase.h"

namespace GameObjectComponent
{
	/**
	 * @brief キャラクターのステータスや無敵状態を管理するコンポーネント
	 */
	class StatusComponent : public IActionComponent, public JsonEditableBase
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param owner このコンポーネントを所有するGameObject
		 */
		StatusComponent(::GameObject* owner);

		/**
		 * @brief デストラクタ
		 */
		~StatusComponent() override;

		/**
		 * @brief 毎フレームの更新処理
		 * @param owner このコンポーネントを所有するGameObject
		 */
		void Update(::GameObject* owner) override;

		/**
		 * @brief ダメージを適用する
		 * @param damage ダメージ値
		 * @return ダメージが実際に適用されたらtrue
		 */
		bool ApplyDamage(int32_t damage);

	public: // ゲッター・セッター
		int32_t GetHp() const { return hp_; }
		void SetHp(int32_t hp) { hp_ = hp; }

		int32_t GetMaxHp() const { return maxHp_; }
		void SetMaxHp(int32_t maxHp) { maxHp_ = maxHp; }

		bool IsAlive() const { return isAlive_; }
		void SetAlive(bool alive) { isAlive_ = alive; }

		bool IsInvincibleMode() const { return isInvincibleMode_; }
		void SetInvincibleMode(bool enable) { isInvincibleMode_ = enable; }

		float GetInvincibleTimer() const { return invincibleTimer_; }
		void SetInvincibleTimer(float duration) { invincibleTimer_ = duration; }

	private:
		// 現在HP
		int32_t hp_ = 100;
		// 最大HP
		int32_t maxHp_ = 100;
		// 生存フラグ
		bool isAlive_ = true;

		// 被弾時の一時的な無敵時間 (秒)
		float invincibleTimer_ = 0.0f;
		// デバッグ用の常時無敵フラグ
		bool isInvincibleMode_ = false;
	};
}
