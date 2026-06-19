#pragma once
/**
 * @file HomingTrailManager.h
 * @brief ホーミングミサイル専用のトレイル管理マネージャー
 */
#include "math/Vector3.h"
#include <cstdint>

class ParticleEffect;
class MultiSourceRibbonModule;
struct Transform;

/**
 * @brief ホーミングミサイルのトレイルを管理するシングルトンクラス
 */
class HomingTrailManager
{
public:
    static HomingTrailManager& GetInstance();

    void Initialize();
    
    /**
     * @brief 弾を登録（手動更新版）
     * @return トレイルID
     */
    uint32_t RegisterBulletManual();

    /**
     * @brief トレイルの座標を手動更新
     * @param trailId 登録時のID
     * @param position 新しい座標
     */
    void UpdateBulletManual(uint32_t trailId, const Vector3& position);

    /**
     * @brief 弾の登録を解除
     * @param trailId 登録時のID
     */
    void UnregisterBullet(uint32_t trailId);

    /**
     * @brief 全トレイルをクリア
     */
    void Clear();

private:
    HomingTrailManager() = default;
    ~HomingTrailManager() = default;
    HomingTrailManager(const HomingTrailManager&) = delete;
    HomingTrailManager& operator=(const HomingTrailManager&) = delete;

private:
    // エフェクト定義名
    static constexpr const char* kEffectName = "HomingTrail";
    // JSONファイルパス
    static constexpr const char* kEffectJsonPath = "./Resources/json/particle/homingTrail.json";

    ParticleEffect* effect_ = nullptr;
    MultiSourceRibbonModule* multiSourceModule_ = nullptr;
    bool initialized_ = false;
};
