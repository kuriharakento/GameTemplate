#pragma once

#include <numbers>
#include <vector>
#include <memory>
#include "application/gameObject/combatable/weapon/Bullet.h"
#include "application/gameObject/component/action/IWeaponComponent.h"

class GameObject;

namespace GameObjectComponent
{
    /**
     * @brief ショットガン武器コンポーネント
     *
     * プレイヤーおよび敵が使用するショットガンの射撃、リロード機能を提供する
     * 1発で複数の弾を扇状に発射する
     */
    class ShotgunComponent : public IWeaponComponent
    {
    public:
        /**
         * @brief コンストラクタ
         * @param object3dCommon 3Dオブジェクト共通情報
         * @param lightManager ライトマネージャー
         */
        ShotgunComponent(Object3dCommon* object3dCommon, LightManager* lightManager);

        /**
         * @brief デストラクタ
         */
        ~ShotgunComponent();

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

        // --- IWeaponComponent インターフェース実装 ---

        /**
         * @brief リロード開始
         */
        void StartReload() override;

        /**
         * @brief 現在の弾数を取得
         * @return 現在の弾数
         */
        int GetCurrentAmmo() const override { return currentAmmo_; }

        /**
         * @brief 最大弾数を取得
         * @return 最大弾数
         */
        int GetMaxAmmo() const override { return maxAmmo_; }

        /**
         * @brief リロード中かどうか
         * @return リロード中ならtrue
         */
        bool IsReloading() const override { return isReloading_; }

        /**
         * @brief リロードの進行度を取得（0.0〜1.0）
         * @return リロード進行度
         */
        float GetReloadProgress() const override
        {
            return isReloading_ ? (reloadTimer_ / reloadTime_) : 0.0f;
        }

        /**
         * @brief 武器名を取得
         * @return 武器の表示名
         */
        const char* GetWeaponName() const override { return "Shotgun"; }

    private:
        // 定数
        // 発射クールダウン時間
        static constexpr float kDefaultFireCooldown = 0.8f;
        // 1発で発射する弾数
        static constexpr int kDefaultPelletCount = 5;
        // 扇状の角度（度数法）
        static constexpr float kDefaultSpreadAngle = 25.0f;
        // 最大弾数
        static constexpr int kDefaultMaxAmmo = 6;
        // リロード時間
        static constexpr float kDefaultReloadTime = 2.5f;
        // 弾の速度
        static constexpr float kBulletSpeed = 20.0f;
        // 弾の寿命
        static constexpr float kBulletLifetime = 1.0f;
        // 発射可能距離（敵用）
        static constexpr float kMaxFireDistance = 25.0f;
        // 弾のスケール
        static constexpr float kBulletScale = 0.3f;
        // Y方向のばらけ範囲
        static constexpr float kVerticalSpreadRange = 0.05f;
        // 度からラジアンへの変換係数
        static constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

        // プレイヤー用の弾発射処理
        void FireBullets(::GameObject* owner);

        // リロード処理
        void Reload(float deltaTime);

        // 3Dオブジェクト共通情報
        Object3dCommon* object3dCommon_ = nullptr;
        // ライトマネージャー
        LightManager* lightManager_ = nullptr;

        // 発射クールダウン時間
        float fireCooldown_;
        // 発射クールダウンタイマー
        float fireCooldownTimer_;
        // 発射された弾のリスト
        std::vector<std::unique_ptr<::Bullet>> bullets_;
        // 1発で発射する弾数
        int pelletCount_ = kDefaultPelletCount;
        // 扇状の角度（度数法）
        float spreadAngle_ = kDefaultSpreadAngle;

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
