#pragma once
#include <cstdint>

namespace ecs
{
    /**
     * @brief プレイヤーの成長状態（スコア、レベル、経験値）を管理する。
     */
    struct PlayerProgressionComponent
    {
        uint64_t totalScore_ = 0;
        
        uint32_t level_ = 1;
        float currentExp_ = 0.0f;
        float nextLevelExp_ = 5.0f;
        
        float expMultiplier_ = 1.0f;
        float scoreMultiplier_ = 1.0f;
    };
}
