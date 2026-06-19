#pragma once
#include "application/gameObject/combatable/base/StatusSystem.h"

namespace ecs
{
/**
 * @brief エンティティのステータス（HP、移動速度など）を管理する。
 */
struct StatusComponent
{
    StatusComponent() = default;
    ~StatusComponent() = default;

    StatusComponent(StatusComponent&&) noexcept = default;
    StatusComponent& operator=(StatusComponent&&) noexcept = default;

    // コピー禁止（StatusValueにunique_ptrが含まれるため）
    StatusComponent(const StatusComponent&) = delete;
    StatusComponent& operator=(const StatusComponent&) = delete;

    StatusValue hp_{ 100.0f };
    StatusValue maxHp_{ 100.0f };
    StatusValue attackPower_{ 10.0f };
    StatusValue moveSpeed_{ 5.0f };
    
    bool isAlive_ = true;
};
}
