#pragma once
#include <cstdint>
#include "engine/ecs/Entity.h"

class ParticleEffect;

namespace ecs
{
    /**
     * @brief タレットの状態を管理する。
     */
    struct TurretComponent
    {
        // 所有プレイヤー。所有しない
        EntityID owner_ = kInvalidEntity;

        float fireInterval_ = 0.2f;
        float fireTimer_ = 0.0f;
        float damage_ = 80.0f;
        float range_ = 30.0f;

        // レーザー用
        EntityID activeBeam_ = kInvalidEntity;
        ParticleEffect* laserEffect_ = nullptr;
    };
}
