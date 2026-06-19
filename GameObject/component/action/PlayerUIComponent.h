#pragma once
#include "engine/gameobject/component/base/IActionComponent.h"
#include "application/ui/GameUI.h"
#include "graphics/2d/NumberSprite.h"
#include <memory>

class SpriteCommon;

namespace GameObjectComponent
{
    class IWeaponComponent;
    class StatusComponent;

    /**
     * @brief プレイヤーのUI表示を担当するコンポーネント
     *
     * 体力バー、弾薬数、リロードインジケーターなどのHUD要素を管理・描画する
     */
    class PlayerUIComponent : public IActionComponent
    {
    public:
        /**
         * @brief コンストラクタ
         * @param spriteCommon スプライト共通設定
         */
        explicit PlayerUIComponent(SpriteCommon* spriteCommon);

        /**
         * @brief デストラクタ
         */
        ~PlayerUIComponent() = default;

        /**
         * @brief 更新処理
         * @param owner 所有するGameObject
         */
        void Update(::GameObject* owner) override;

        /**
         * @brief 描画処理（3D）※このコンポーネントでは何もしない
         * @param camera カメラマネージャー
         */
        void Draw3D(CameraManager* camera) override {}

        /**
         * @brief 描画処理（2D UI）
         */
        void Draw2D() override;

        /*---------------[ ゲッター ]---------------*/

        /**
         * @brief 体力バーの表示状態を取得
         * @return 表示中ならtrue
         */
        bool IsHealthBarVisible() const { return isHealthBarVisible_; }

        /**
         * @brief 弾薬表示の表示状態を取得
         * @return 表示中ならtrue
         */
        bool IsAmmoDisplayVisible() const { return isAmmoDisplayVisible_; }

        /**
         * @brief リロードインジケーターの表示状態を取得
         * @return 表示中ならtrue
         */
        bool IsReloadIndicatorVisible() const { return isReloadIndicatorVisible_; }

        /**
         * @brief バレットタイムクールダウンゲージの表示状態を取得
         * @return 表示中ならtrue
         */
        bool IsBulletTimeBarVisible() const { return isBulletTimeBarVisible_; }

        /*---------------[ セッター ]---------------*/

        /**
         * @brief 体力バーの表示/非表示を設定
         * @param isVisible 表示するならtrue
         */
        void SetHealthBarVisible(bool isVisible) { isHealthBarVisible_ = isVisible; }

        /**
         * @brief 弾薬表示の表示/非表示を設定
         * @param isVisible 表示するならtrue
         */
        void SetAmmoDisplayVisible(bool isVisible) { isAmmoDisplayVisible_ = isVisible; }

        /**
         * @brief リロードインジケーターの表示/非表示を設定
         * @param isVisible 表示するならtrue
         */
        void SetReloadIndicatorVisible(bool isVisible) { isReloadIndicatorVisible_ = isVisible; }

        /**
         * @brief バレットタイムクールダウンゲージの表示/非表示を設定
         * @param isVisible 表示するならtrue
         */
        void SetBulletTimeBarVisible(bool isVisible) { isBulletTimeBarVisible_ = isVisible; }

        /**
         * @brief 全UIの表示/非表示を設定
         * @param isVisible 表示するならtrue
         */
        void SetAllVisible(bool isVisible);

    private:
        // 定数
        // 体力バーの位置
        static constexpr float kHealthBarPosX = 50.0f;
        static constexpr float kHealthBarPosY = 650.0f;
        // 体力バーのサイズ
        static constexpr float kHealthBarWidth = 300.0f;
        static constexpr float kHealthBarHeight = 40.0f;
        // 弾薬表示の位置
        static constexpr float kAmmoPosX = 1100.0f;
        static constexpr float kAmmoPosY = 650.0f;
        static constexpr float kAmmoNumberSpacing = -32.0f; // 弾薬数の桁間スペース
        // 弾薬アイコンのサイズ
        static constexpr float kAmmoIconSize = 50.0f;
        // リロードインジケーターの位置
        static constexpr float kReloadPosX = 640.0f;
        static constexpr float kReloadPosY = 500.0f;
        // リロードバーのサイズ
        static constexpr float kReloadBarWidth = 200.0f;
        static constexpr float kReloadBarHeight = 10.0f;

        // バレットタイムゲージの位置
        static constexpr float kBulletTimePosX = 640.0f;
        static constexpr float kBulletTimePosY = 540.0f;
        // バレットタイムゲージのサイズ
        static constexpr float kBulletTimeBarWidth = 200.0f;
        static constexpr float kBulletTimeBarHeight = 10.0f;

        // 数字1桁のサイズ
        static constexpr float kDigitWidth = 64.0f;
        static constexpr float kDigitHeight = 64.0f;

        /**
         * @brief UI要素の初期化
         */
        void InitializeUI();

        /**
         * @brief 他のコンポーネントへの参照をキャッシュ
         * @param owner 所有するGameObject
         */
        void CacheComponents(::GameObject* owner);

        /**
         * @brief 体力バーの更新
         */
        void UpdateHealthBar();

        /**
         * @brief 弾薬表示の更新
         * @param weapon 現在の武器
         */
        void UpdateAmmoDisplay(IWeaponComponent* weapon);

        /**
         * @brief リロードインジケーターの更新
         * @param weapon 現在の武器
         */
        void UpdateReloadIndicator(IWeaponComponent* weapon);

        /**
         * @brief バレットタイムインジケーターの更新
         * @param owner 所有するGameObject
         */
        void UpdateBulletTimeIndicator(::GameObject* owner);

    private:
        // スプライト共通設定
        SpriteCommon* spriteCommon_ = nullptr;

        // UI要素
        std::unique_ptr<GameUI> healthBarBg_;        // 体力バー背景
        std::unique_ptr<GameUI> healthBarFill_;      // 体力バー（可変）
        std::unique_ptr<GameUI> ammoIcon_;           // 弾薬アイコン
        std::unique_ptr<NumberSprite> ammoNumber_;   // 弾薬数表示
        std::unique_ptr<GameUI> reloadBarBg_;        // リロードバー背景
        std::unique_ptr<GameUI> reloadBarFill_;      // リロードバー（可変）
        std::unique_ptr<GameUI> bulletTimeBarBg_;    // バレットタイムバー背景
        std::unique_ptr<GameUI> bulletTimeBarFill_;  // バレットタイムバー（可変）

        // キャッシュしたコンポーネント参照
        std::weak_ptr<StatusComponent> statusComp_;

        // 初期化フラグ
        // 表示フラグ
        bool isInitialized_ = false;
        bool isHealthBarVisible_ = true;
        bool isAmmoDisplayVisible_ = true;
        bool isReloadIndicatorVisible_ = true;
        bool isBulletTimeBarVisible_ = true;

        // 体力バーの最大幅（初期化時に保存）
        float healthBarMaxWidth_ = kHealthBarWidth;
    };
}