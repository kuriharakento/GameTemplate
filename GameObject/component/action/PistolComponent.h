#pragma once
#include <memory>
#include <vector>

#include "application/gameObject/combatable/weapon/Bullet.h"
#include "engine/gameobject/component/base/IActionComponent.h"
#include "input/Input.h"
#include "math/MathUtils.h"
#include "math/Vector3.h"

class GameObject;

namespace GameObjectComponent
{
	/**
	 * @brief ピストル武器コンポーネント
	 *
	 * プレイヤーおよび敵が使用するピストルの射撃、リロード機能を提供する
	 */
	class PistolComponent : public IActionComponent
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param object3dCommon 3Dオブジェクト共通情報
		 * @param lightManager ライトマネージャー
		 */
		PistolComponent(Object3dCommon* object3dCommon, LightManager* lightManager);

		/**
		 * @brief デストラクタ
		 */
		~PistolComponent();

		/**
		 * @brief フレームごとの更新処理
		 * @param owner このコンポーネントを所有するゲームオブジェクト
		 */
		void Update(::GameObject* owner) override;

		/**
		 * @brief 描画処理
		 * @param camera カメラマネージャー
		 */
		void Draw3D(CameraManager* camera) override;

	private:
		// 定数
		// 発射クールダウン時間
		static constexpr float kDefaultFireCooldown = 0.5f;
		// 最大弾数
		static constexpr int kDefaultMaxAmmo = 12;
		// リロード時間
		static constexpr float kDefaultReloadTime = 1.5f;
		// 弾の速度
		static constexpr float kBulletSpeed = 30.0f;
		// 弾の寿命
		static constexpr float kBulletLifetime = 2.0f;
		// 発射可能距離（敵用）
		static constexpr float kMaxFireDistance = 30.0f;
		// 弾のスケール
		static constexpr float kBulletScale = 0.3f;

		// プレイヤー用の弾発射処理
		void FireBullet(::GameObject* owner);

		// リロード開始
		void StartReload();
		// リロード処理
		void Reload(float deltaTime);

		// 3Dオブジェクト共通情報
		Object3dCommon* object3dCommon_ = nullptr;
		// ライトマネージャー
		LightManager* lightManager_ = nullptr;

		// 発射のクールダウン時間
		float fireCooldown_;
		// 現在のクールダウンタイマー
		float fireCooldownTimer_;
		// 発射された弾のリスト
		std::vector<std::unique_ptr<::Bullet>> bullets_;

		// マガジン最大弾数
		int maxAmmo_ = kDefaultMaxAmmo;
		// 現在の弾数
		int currentAmmo_ = kDefaultMaxAmmo;
		// リロード中か
		bool isReloading_ = false;
		// リロードにかかる時間
		float reloadTime_ = kDefaultReloadTime;
		// リロード経過時間
		float reloadTimer_ = 0.0f;
	};
}
