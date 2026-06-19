#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "engine/ecs/Entity.h"

class ParticleEffect;

namespace ecs
{
    /**
     * @brief スキルの基本ルート。
     */
    enum class SkillRoute : uint8_t
    {
        None,
        Bomb,   // ボムルート（Q: 衝撃波）
        Turret  // タレットルート（Q: 設置）
    };

    /**
     * @brief スキルの派生。
     */
    enum class SkillSpecialChoice : uint8_t
    {
        None,
        // ボムルート派生
        HomingMissile,
        DecoyBomb,
        // タレットルート派生
        MissileSalvo,
        PlasmaLaser
    };

    /**
     * @brief プレイヤースキルの状態・クールタイムを管理する。
     */
    struct SkillComponent
    {
        // --- ルート設定 ---
        SkillRoute route_ = SkillRoute::None;
        SkillSpecialChoice special_ = SkillSpecialChoice::None;

        // --- LMB: 通常攻撃 ---
        bool isLmbUnlocked_ = true;
        float lmbTimer_ = 0.0f;
        static constexpr float kLmbBaseCooldown = 0.9f;
        float lmbDamageMultiplier_ = 1.0f;
        float lmbCooldownMultiplier_ = 1.0f;
        
        // 通常弾メカニズム
        uint32_t lmbPierceCount_ = 0;
        uint32_t lmbBulletCount_ = 1;
        float lmbProjectileSpeed_ = 80.0f;

        // --- Q: ベーススキル ---
        float baseSkillTimer_ = 0.0f;
        static constexpr float kBaseSkillCooldown = 2.0f;
        
        // Qスキル強化項目
        float qRange_ = 15.0f;
        float qCooldownMultiplier_ = 1.0f;

        // --- E: 派生スキル ---
        float specialSkillTimer_ = 0.0f;
        static constexpr float kSpecialSkillCooldown = 5.0f;
        float eCooldownMultiplier_ = 1.0f;

        // Eスキル個別強化項目
        uint32_t eMissileCount_ = 5;
        float eDecoyDuration_ = 10.0f;
        float eSalvoDamageMult_ = 1.0f;
        float eLaserFireRateMult_ = 0.1f;

        // --- 設置物管理 ---
        std::vector<EntityID> activeDecoyEntities_;
        std::vector<EntityID> activeTurretEntities_;
        uint32_t qMaxTurrets_ = 1;
        float qTurretFireRateMult_ = 1.0f;

        // 派生状態フラグ
        bool isTurretBuffActive_ = false;
        float turretBuffTimer_ = 0.0f;
        static constexpr float kTurretBuffDuration = 3.0f;

        // --- R: ビーム ---
        bool isBeamUnlocked_ = false;
        float beamCharge_ = 0.0f;
        bool isBeamReady_ = false;
        static constexpr float kBeamChargeMax = 30.0f;
        static constexpr float kChargePerKill = 1.0f;

        // ビーム再生管理
        ParticleEffect* activeBeamParticle_ = nullptr;
        float beamActiveTimer_ = 0.0f;
    };
}
