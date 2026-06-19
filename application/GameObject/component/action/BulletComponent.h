#pragma once
#include "engine/gameobject/component/base/IActionComponent.h"
#include "math/Vector3.h"

/**
 * @brief 弾丸の移動と寿命を管理するコンポーネント
 *
 * 弾丸の進行方向、速度、寿命を設定し、フレームごとに位置を更新する
 */
namespace GameObjectComponent
{
	class BulletComponent : public IActionComponent
	{
	public:
		/**
		 * @brief コンストラクタ
		 */
		BulletComponent();

		/**
		 * @brief 弾丸の初期化
		 * @param direction 弾丸の進行方向
		 * @param speed 弾丸の移動速度
		 * @param lifetime 弾丸の寿命（秒）
		 */
		void Initialize(const Vector3& direction, float speed, float lifetime);

		/**
		 * @brief フレームごとの更新処理
		 * @param owner このコンポーネントを所有するゲームオブジェクト
		 */
		void Update(::GameObject* owner) override;

	private:
		// 弾の進行方向
		Vector3 direction_;
		// 弾の移動速度
		float speed_;
		// 弾の寿命
		float lifetime_;
		// 経過時間
		float timeAlive_;
		// タイムスケールを無視するかどうか
		bool ignoreTimeScale_ = false;

	public:
		/**
		 * @brief タイムスケールを無視するかどうかを設定
		 * @param ignore 無視する場合は true
		 */
		void SetIgnoreTimeScale(bool ignore) { ignoreTimeScale_ = ignore; }
	};
}

