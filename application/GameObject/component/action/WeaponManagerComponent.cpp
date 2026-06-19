#include "WeaponManagerComponent.h"
#include "AssaultRifleComponent.h"
#include "ShotgunComponent.h"
#include "input/Input.h"
#include "engine/gameobject/base/GameObject.h"
#include "application/GameObject/Combatable/character/player/Player.h"

namespace GameObjectComponent
{
    WeaponManagerComponent::WeaponManagerComponent(Object3dCommon* object3dCommon, LightManager* lightManager)
        : object3dCommon_(object3dCommon), lightManager_(lightManager)
    {
    }

    void WeaponManagerComponent::AddWeapon(std::unique_ptr<IWeaponComponent> weapon)
    {
        if (weapon)
        {
            weapons_.push_back(std::move(weapon));
        }
    }

    void WeaponManagerComponent::SwitchToNext()
    {
        if (weapons_.empty()) return;
        SwitchTo((currentIndex_ + 1) % static_cast<int>(weapons_.size()));
    }

    void WeaponManagerComponent::SwitchToPrevious()
    {
        if (weapons_.empty()) return;
        SwitchTo((currentIndex_ - 1 + static_cast<int>(weapons_.size())) % static_cast<int>(weapons_.size()));
    }

    void WeaponManagerComponent::SwitchTo(int index)
    {
        if (index >= 0 && index < static_cast<int>(weapons_.size()))
        {
            currentIndex_ = index;
        }
    }

    void WeaponManagerComponent::ProcessSwitchInput()
    {
        auto input = Input::GetInstance();

        // 数字キーで切り替え（1〜3）
        if (input->TriggerKey(DIK_1) && weapons_.size() >= 1) SwitchTo(0);
        if (input->TriggerKey(DIK_2) && weapons_.size() >= 2) SwitchTo(1);
        if (input->TriggerKey(DIK_3) && weapons_.size() >= 3) SwitchTo(2);

        // Qキーで前の武器
        if (input->TriggerKey(DIK_Q)) SwitchToPrevious();
    }

    void WeaponManagerComponent::Update(::GameObject* owner)
    {
        // プレイヤーの場合のみ切り替え入力を処理
        if (dynamic_cast<::Player*>(owner))
        {
            ProcessSwitchInput();
        }

        // 全武器のUpdateを呼ぶ（弾の更新は全武器で必要）
        // ただし入力処理は選択中の武器のみ行うよう、isActive_フラグを設定
        for (size_t i = 0; i < weapons_.size(); ++i)
        {
            weapons_[i]->SetActive(static_cast<int>(i) == currentIndex_);
            weapons_[i]->Update(owner);
        }
    }

    void WeaponManagerComponent::Draw3D(CameraManager* camera)
    {
        // 全武器の弾を描画（発射済みの弾は見せる必要がある）
        for (const auto& weapon : weapons_)
        {
            weapon->Draw3D(camera);
        }
    }

    IWeaponComponent* WeaponManagerComponent::GetCurrentWeapon() const
    {
        if (weapons_.empty() || currentIndex_ < 0 || currentIndex_ >= static_cast<int>(weapons_.size()))
        {
            return nullptr;
        }
        return weapons_[currentIndex_].get();
    }
}
