#pragma once
#include "math/Vector3.h"

namespace ecs
{
    /**
     * @brief 回避（Dodge）アクションの状態とパラメータを管理する。
     */
    struct DodgeComponent
    {
        bool isDodging_ = false;
        float timer_ = 0.0f;
        float cooldownTimer_ = 0.0f;

        Vector3 direction_ = { 0, 0, 0 };
        Vector3 startPosition_ = { 0, 0, 0 };
        Vector3 targetPosition_ = { 0, 0, 0 };

        static constexpr float kDefaultDistance = 8.0f;
        static constexpr float kDefaultDuration = 0.25f;
        static constexpr float kDefaultCooldown = 0.8f;
        static constexpr float kDefaultInvincibleTime = 0.25f;
    };
}
