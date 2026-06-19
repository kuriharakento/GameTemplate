#pragma once
#include "application/gameObject/combatable/weapon/Bullet.h"
#include "application/gameObject/component/action/IWeaponComponent.h"


/**
 * @brief アサルトライフル武器コンポーネント
 *
 * プレイヤーおよび敵が使用するアサルトライフルの射撃、リロード機能を提供する
 */
namespace GameObjectComponent
{
	class AssaultRifleComponent : public IWeaponComponent
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param object3dCommon 3Dオブジェクト共通情報
		 * @param lightManager ライトマネージャー
		 */
		AssaultRifleComponent(::Object3dCommon* object3dCommon, ::LightManager* lightManager);

		/**
		 * @brief デストラクタ
		 */
		~AssaultRifleComponent();

		/**
		 * @brief フレームごとの更新処理
		 * @param owner このコンポーネントを所有するゲームオブジェクト
		 */
		void Update(::GameObject* owner) override;

		/**
		 * @brief 描画処理
		 * @param camera カメラマネージャー
		 */
		void Draw3D(::CameraManager* camera) override;


		/**
		 * @brief 発射された弾のリストを取得する
		 * @return 弾のリスト
		 */
		const std::vector<std::unique_ptr<::Bullet>>& GetBullets() { return bullets_; }

		/**
		 * @brief 現在の弾数を取得
		 * @return 現在の弾数
		 */
		int GetCurrentAmmo() const { return currentAmmo_; }

		/**
		 * @brief 最大弾数を取得
		 * @return 最大弾数
		 */
		int GetMaxAmmo() const { return maxAmmo_; }

		/**
		 * @brief リロード中かどうかを取得
		 * @return リロード中ならtrue
		 */
		bool IsReloading() const { return isReloading_; }

		/**
		 * @brief リロードの進行度を取得（0.0〜1.0）
		 * @return リロード進行度
		 */
		float GetReloadProgress() const
		{
			return isReloading_ ? (reloadTimer_ / reloadTime_) : 0.0f;
		}

		/**
		 * @brief リロード時間を取得
		 * @return リロード所要時間（秒）
		 */
		float GetReloadTime() const { return reloadTime_; }

		/**
		 * @brief 武器名を取得
		 * @return 武器の表示名
		 */
		const char* GetWeaponName() const override { return "Assault Rifle"; }

		/**
		 * @brief リロード開始
		 */
		void StartReload() override;

	private:
		// 定数
		// 発射クールダウン時間
		static constexpr float kDefaultFireCooldown = 0.1f;
		// 最大弾数
		static constexpr int kDefaultMaxAmmo = 30;
		// リロード時間
		static constexpr float kDefaultReloadTime = 2.0f;
		// 弾の速度
		static constexpr float kDefaultBulletSpeed = 50.0f;
		// 弾の寿命
		static constexpr float kDefaultBulletLifetime = 2.0f;
		// 発射可能距離
		static constexpr float kMaxFireDistance = 40.0f;
		// 弾のスケール
		static constexpr float kBulletScale = 0.3f;

		// プレイヤー用の弾発射処理
		void FireBullet(::GameObject* owner);

		// リロード処理
		void Reload(float deltaTime);

		// 3Dオブジェクト共通情報
		::Object3dCommon* object3dCommon_ = nullptr;
		// ライトマネージャー
		::LightManager* lightManager_ = nullptr;

		// 発射クールダウン時間
		float fireCooldown_;
		// 発射クールダウンタイマー
		float fireCooldownTimer_;
		// 発射された弾のリスト
		std::vector<std::unique_ptr<::Bullet>> bullets_;

		// 最大弾数
		int maxAmmo_ = kDefaultMaxAmmo;
		// 現在の弾数
		int currentAmmo_ = kDefaultMaxAmmo;
		// リロード中フラグ
		bool isReloading_ = false;
		// リロード所要時間
		float reloadTime_ = kDefaultReloadTime;
		// リロード経過時間
		float reloadTimer_ = 0.0f;
		// 弾の速度
		float speed_ = kDefaultBulletSpeed;
		// 弾の寿命
		float lifetime_ = kDefaultBulletLifetime;

	};
}

