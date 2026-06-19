#pragma once
#include "math/Vector3.h"

namespace ecs
{
    /**
     * @brief プレイヤーのステータスやリソースを管理する。
     */
    struct PlayerComponent
    {
        // スコア・経験値
        uint64_t score_ = 0;
        uint32_t level_ = 1;
        float exp_ = 0.0f;
        float nextLevelExp_ = 100.0f;
        
        // 無敵管理
        bool isInvincible_ = false;
        float invincibleTimer_ = 0.0f;
        
        // 倍率パラメータ
        float attackMultiplier_ = 1.0f;
        float moveSpeedMultiplier_ = 1.0f;

        // 移動・回転
        float rotationSpeed_ = 0.1f;
        bool hasMovementInput_ = false;
        
        // 回避動作
        bool isDodging_ = false;
        float dodgeTimer_ = 0.0f;
        float dodgeCooldownTimer_ = 0.0f;
        Vector3 dodgeDirection_ = { 0, 0, 0 };
        Vector3 dodgeStartPosition_ = { 0, 0, 0 };
        Vector3 dodgeTargetPosition_ = { 0, 0, 0 };

        // 設定定数
        static constexpr float kDefaultDodgeDistance = 8.0f;
        static constexpr float kDefaultDodgeDuration = 0.25f;
        static constexpr float kDefaultDodgeCooldown = 1.5f;
        static constexpr float kDefaultDodgeInvincibleTime = 0.25f;
    };
}
