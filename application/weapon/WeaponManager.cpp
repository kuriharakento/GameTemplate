#include "WeaponManager.h"
#include "input/Input.h"

using namespace GameObjectComponent;

WeaponManager::WeaponManager(GameObject* owner)
    : owner_(owner)
{
}

void WeaponManager::AddWeapon(IWeaponComponent* weapon)
{
    if (weapon)
    {
        // 最初の武器以外は無効にする
        if (!weapons_.empty())
        {
            weapon->SetActive(false);
        }
        weapons_.push_back(weapon);
    }
}

void WeaponManager::SwitchToNext()
{
    if (weapons_.empty()) return;
    SwitchTo((currentIndex_ + 1) % static_cast<int>(weapons_.size()));
}

void WeaponManager::SwitchToPrevious()
{
    if (weapons_.empty()) return;
    SwitchTo((currentIndex_ - 1 + static_cast<int>(weapons_.size())) % static_cast<int>(weapons_.size()));
}

void WeaponManager::SwitchTo(int index)
{
    if (index >= 0 && index < static_cast<int>(weapons_.size()) && index != currentIndex_)
    {
        // 現在の武器を無効化
        if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(weapons_.size()))
        {
            weapons_[currentIndex_]->SetActive(false);
        }

        // 新しい武器を有効化
        currentIndex_ = index;
        weapons_[currentIndex_]->SetActive(true);
    }
}

void WeaponManager::Update()
{
    auto input = Input::GetInstance();

    // 数字キーで切り替え（1〜3）
    if (input->TriggerKey(DIK_1) && weapons_.size() >= 1) SwitchTo(0);
    if (input->TriggerKey(DIK_2) && weapons_.size() >= 2) SwitchTo(1);
    if (input->TriggerKey(DIK_3) && weapons_.size() >= 3) SwitchTo(2);

    // Qキーで前の武器
    if (input->TriggerKey(DIK_Q)) SwitchToPrevious();
}

IWeaponComponent* WeaponManager::GetCurrentWeapon() const
{
    if (weapons_.empty() || currentIndex_ < 0 || currentIndex_ >= static_cast<int>(weapons_.size()))
    {
        return nullptr;
    }
    return weapons_[currentIndex_];
}
