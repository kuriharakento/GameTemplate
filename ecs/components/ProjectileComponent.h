#pragma once
#include "math/Vector3.h"
#include "engine/ecs/Entity.h"

namespace ecs
{
    /**
     * @brief 弾丸（Projectile）の性質を管理する。
     */
    struct ProjectileComponent
    {
        /**
         * @brief 弾丸タイプ。
         */
        enum class Type
        {
            Lmb,
            Rmb,
            Decoy,
            Impact,
            Beam
        };

        /**
         * @brief トレイルタイプ。
         */
        enum class TrailType
        {
            Bullet,
            Homing,
            None
        };
        
        Vector3 velocity_ = { 0, 0, 0 };
        float speed_ = 50.0f;
        float damage_ = 10.0f;
        float currentRadius_ = 0.5f;

        float lifetime_ = 2.0f;
        float currentLifetime_ = 0.0f;
        bool isAlive_ = true;

        Type type_ = Type::Lmb;
        TrailType trailType_ = TrailType::Bullet;
        uint32_t trailId_ = 0;
        uint32_t pierceCount_ = 0;

        EntityID targetEntity_ = kInvalidEntity;
        bool isHoming_ = false;
    };
}
