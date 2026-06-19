#pragma once
#include "engine/ecs/Entity.h"

class ParticleEffect;

namespace ecs
{
    /**
     * @brief デコイを管理する。
     */
    struct DecoyComponent
    {
        // 生成元のプレイヤー。所有しない
        EntityID owner_ = kInvalidEntity;

        // 再生中のエフェクト。消滅時に停止させるため
        ParticleEffect* effect_ = nullptr;
    };
}
