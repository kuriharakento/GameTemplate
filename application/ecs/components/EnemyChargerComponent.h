#pragma once
#include "math/Vector3.h"

namespace ecs
{
    /**
     * @brief 突進型の敵（Charger）専用の状態管理。
     */
    struct EnemyChargerComponent
    {
        // ステート（0: 待機, 1: 突進, 2: 硬直）
        int state_ = 0;
        float timer_ = 0.0f;
        Vector3 chargeDirection_ = { 0.0f, 0.0f, 0.0f };
    };
}
