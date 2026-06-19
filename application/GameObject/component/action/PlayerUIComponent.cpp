#include "PlayerUIComponent.h"

#include "engine/gameobject/base/GameObject.h"
#include "application/GameObject/Combatable/character/player/Player.h"
#include "application/GameObject/component/action/StatusComponent.h"
#include "application/GameObject/component/action/IWeaponComponent.h"
#include "application/GameObject/component/action/MoveComponent.h"
#include "math/VectorColorCodes.h"

// HP残量の閾値
constexpr float kHpThresholdHigh = 0.5f;
constexpr float kHpThresholdLow = 0.25f;

// 体力バーの色
const Vector4 kHealthColorHigh = VectorColorCodes::Green;       // 緑
const Vector4 kHealthColorMedium = VectorColorCodes::Yellow;    // 黄
const Vector4 kHealthColorLow = VectorColorCodes::Red;          // 赤

// リロードバーの色
const Vector4 kReloadBarColor = VectorColorCodes::Yellow;       // 黄

// バレットタイムバーの色
const Vector4 kBulletTimeBarColor = VectorColorCodes::SkyBlue;  // 水色

namespace GameObjectComponent
{
    PlayerUIComponent::PlayerUIComponent(SpriteCommon* spriteCommon)
        : spriteCommon_(spriteCommon)
    {
        InitializeUI();
    }

    void PlayerUIComponent::InitializeUI()
    {
        // 体力バー背景
        healthBarBg_ = std::make_unique<GameUI>();
        healthBarBg_->Initialize(spriteCommon_, "./Resources/UI/hp_bar_frame.png");
        healthBarBg_->SetScreenPosition({ kHealthBarPosX, kHealthBarPosY });
        healthBarBg_->SetSize({ kHealthBarWidth, kHealthBarHeight });
        healthBarBg_->SetTextureSize({ kHealthBarWidth, kHealthBarHeight });
        healthBarBg_->SetAnchorPoint({ 0.0f, 0.5f });
        healthBarBg_->SetInteractable(false);

        // 体力バー
        healthBarFill_ = std::make_unique<GameUI>();
        healthBarFill_->Initialize(spriteCommon_, "./Resources/UI/hp_bar_fill.png");
        healthBarFill_->SetScreenPosition({ kHealthBarPosX, kHealthBarPosY });
        healthBarFill_->SetSize({ kHealthBarWidth, kHealthBarHeight });
        healthBarFill_->SetTextureSize({ kHealthBarWidth, kHealthBarHeight });
        healthBarFill_->SetAnchorPoint({ 0.0f, 0.5f });
        healthBarFill_->SetInteractable(false);

        // 弾薬アイコン
        ammoIcon_ = std::make_unique<GameUI>();
        ammoIcon_->Initialize(spriteCommon_, "./Resources/UI/remaining_ammo.png");
        ammoIcon_->SetSize({ kAmmoIconSize, kAmmoIconSize });
        ammoIcon_->SetScreenPosition({ kAmmoPosX - 115.0f, kAmmoPosY });
        ammoIcon_->SetInteractable(false);

        // 弾薬数表示
        ammoNumber_ = std::make_unique<NumberSprite>();
        ammoNumber_->Initialize(spriteCommon_, "./Resources/numbers.png", { kDigitWidth, kDigitHeight });
        ammoNumber_->SetPosition({ kAmmoPosX, kAmmoPosY });
        ammoNumber_->SetSpacing(kAmmoNumberSpacing);

        // リロードバー背景
        reloadBarBg_ = std::make_unique<GameUI>();
        reloadBarBg_->Initialize(spriteCommon_, "./Resources/white1x1.png");
        reloadBarBg_->SetScreenPosition({ kReloadPosX, kReloadPosY });
        reloadBarBg_->SetSize({ kReloadBarWidth, kReloadBarHeight });
        reloadBarBg_->SetAnchorPoint({ 0.5f, 0.5f });
        reloadBarBg_->SetInteractable(false);
        reloadBarBg_->SetVisible(false);

        // リロードバー
        reloadBarFill_ = std::make_unique<GameUI>();
        reloadBarFill_->Initialize(spriteCommon_, "./Resources/white1x1.png");
        reloadBarFill_->SetScreenPosition({ kReloadPosX - kReloadBarWidth * 0.5f, kReloadPosY });
        reloadBarFill_->SetSize({ 0.0f, kReloadBarHeight });
        reloadBarFill_->SetAnchorPoint({ 0.0f, 0.5f });
        reloadBarFill_->SetInteractable(false);
        reloadBarFill_->SetColor(kReloadBarColor);
        reloadBarFill_->SetVisible(false);

        // バレットタイムバー背景
        bulletTimeBarBg_ = std::make_unique<GameUI>();
        bulletTimeBarBg_->Initialize(spriteCommon_, "./Resources/white1x1.png");
        bulletTimeBarBg_->SetScreenPosition({ kBulletTimePosX, kBulletTimePosY });
        bulletTimeBarBg_->SetSize({ kBulletTimeBarWidth, kBulletTimeBarHeight });
        bulletTimeBarBg_->SetAnchorPoint({ 0.5f, 0.5f });
        bulletTimeBarBg_->SetInteractable(false);
        bulletTimeBarBg_->SetVisible(false);
        
        // バレットタイムバー
        bulletTimeBarFill_ = std::make_unique<GameUI>();
        bulletTimeBarFill_->Initialize(spriteCommon_, "./Resources/white1x1.png");
        bulletTimeBarFill_->SetScreenPosition({ kBulletTimePosX - kBulletTimeBarWidth * 0.5f, kBulletTimePosY });
        bulletTimeBarFill_->SetSize({ 0.0f, kBulletTimeBarHeight });
        bulletTimeBarFill_->SetAnchorPoint({ 0.0f, 0.5f });
        bulletTimeBarFill_->SetInteractable(false);
        bulletTimeBarFill_->SetColor(kBulletTimeBarColor);
        bulletTimeBarFill_->SetVisible(false);
    }

    void PlayerUIComponent::CacheComponents(::GameObject* owner)
    {
        if (isInitialized_) return;

        statusComp_ = owner->GetComponent<StatusComponent>();
        isInitialized_ = true;
    }

    void PlayerUIComponent::Update(::GameObject* owner)
    {
        CacheComponents(owner);

        // Playerから現在の武器を取得
        IWeaponComponent* currentWeapon = nullptr;
        if (auto* player = dynamic_cast<::Player*>(owner))
        {
            currentWeapon = player->GetCurrentWeapon();
        }

        UpdateHealthBar();
        UpdateAmmoDisplay(currentWeapon);
        UpdateReloadIndicator(currentWeapon);
        UpdateBulletTimeIndicator(owner);

        // 各UI要素の更新
        if (isHealthBarVisible_)
        {
            healthBarBg_->Update();
            healthBarFill_->Update();
        }

        if (isAmmoDisplayVisible_)
        {
            ammoIcon_->Update();
            ammoNumber_->Update();
        }

        if (isReloadIndicatorVisible_)
        {
            reloadBarBg_->Update();
            reloadBarFill_->Update();
        }

        if (isBulletTimeBarVisible_)
        {
            bulletTimeBarBg_->Update();
            bulletTimeBarFill_->Update();
        }
    }

    void PlayerUIComponent::Draw2D()
    {
        // 体力バー
        if (isHealthBarVisible_)
        {
            healthBarBg_->Draw();
            if (healthBarFill_->GetSize().x > 0)
            {
                healthBarFill_->Draw();
            }
        }

        // 弾薬表示
        if (isAmmoDisplayVisible_)
        {
            ammoIcon_->Draw();
            ammoNumber_->Draw();
        }

        // リロードインジケーター
        if (isReloadIndicatorVisible_)
        {
            reloadBarBg_->Draw();
            reloadBarFill_->Draw();
        }

        // バレットタイムインジケーター
        if (isBulletTimeBarVisible_)
        {
            bulletTimeBarBg_->Draw();
            bulletTimeBarFill_->Draw();
        }
    }

    void PlayerUIComponent::UpdateHealthBar()
    {
        auto status = statusComp_.lock();
        if (!status) return;

        // HP比率を計算
        float hpRatio = status->hp.GetValue() / status->maxHp.GetValue();
        if (hpRatio < 0.0f) hpRatio = 0.0f;
        if (hpRatio > 1.0f) hpRatio = 1.0f;

        // 体力バーの幅を更新
        healthBarFill_->SetSize({ healthBarMaxWidth_ * hpRatio, kHealthBarHeight });
        healthBarFill_->SetTextureSize({ healthBarMaxWidth_ * hpRatio, kHealthBarHeight });

        // HP残量に応じて色を変更
        if (hpRatio > kHpThresholdHigh)
        {
            healthBarFill_->SetColor(kHealthColorHigh);
        }
        else if (hpRatio > kHpThresholdLow)
        {
            healthBarFill_->SetColor(kHealthColorMedium);
        }
        else
        {
            healthBarFill_->SetColor(kHealthColorLow);
        }
    }

    void PlayerUIComponent::UpdateAmmoDisplay(IWeaponComponent* weapon)
    {
        if (!weapon) return;

        // 弾数を更新
        ammoNumber_->SetNumber(weapon->GetCurrentAmmo());
    }

    void PlayerUIComponent::UpdateReloadIndicator(IWeaponComponent* weapon)
    {
        if (!weapon)
        {
            reloadBarBg_->SetVisible(false);
            reloadBarFill_->SetVisible(false);
            return;
        }

        bool isReloading = weapon->IsReloading();

        // リロード中のみ表示
        reloadBarBg_->SetVisible(isReloading);
        reloadBarFill_->SetVisible(isReloading);

        if (isReloading)
        {
            // リロード進行度を計算
            float progress = weapon->GetReloadProgress();
            reloadBarFill_->SetSize({ kReloadBarWidth * progress, kReloadBarHeight });
        }
    }

    void PlayerUIComponent::UpdateBulletTimeIndicator(::GameObject* owner)
    {
        auto moveComp = owner->GetComponent<MoveComponent>();
        if (!moveComp)
        {
            bulletTimeBarBg_->SetVisible(false);
            bulletTimeBarFill_->SetVisible(false);
            return;
        }

        // リロード機能と同じ仕様にするため、クールダウン中のみ表示する
        bool isCoolingDown = moveComp->IsBulletTimeCoolingDown();

        bulletTimeBarBg_->SetVisible(isCoolingDown);
        bulletTimeBarFill_->SetVisible(isCoolingDown);

        if (isCoolingDown)
        {
            // 進行度を取得 (0.0 -> 1.0と増えていく)
            float progress = moveComp->GetBulletTimeCooldownProgress();
            bulletTimeBarFill_->SetSize({ kBulletTimeBarWidth * progress, kBulletTimeBarHeight });
        }
    }

    void PlayerUIComponent::SetAllVisible(bool isVisible)
    {
        isHealthBarVisible_ = isVisible;
        isAmmoDisplayVisible_ = isVisible;
        isReloadIndicatorVisible_ = isVisible;
        isBulletTimeBarVisible_ = isVisible;
    }
}