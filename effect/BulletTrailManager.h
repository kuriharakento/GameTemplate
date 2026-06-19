#pragma once
/**
 * @file BulletTrailManager.h
 * @brief 弾道トレイル管理マネージャー（最適化版）
 *
 * 単一のParticleEffect内のMultiSourceRibbonModuleを使用して、
 * 複数の弾丸トレイルを1回のドローコールで描画。
 */
#include <unordered_map>
#include <string>
#include "math/Vector3.h"

class ParticleEffect;
class ParticleEmitter;
class MultiSourceRibbonModule;
struct Transform;

/**
 * @brief 弾道トレイル管理マネージャー（最適化版）
 *
 * 単一のエフェクト内のMultiSourceRibbonModuleを使用して、
 * 複数の弾丸トレイルを効率的に管理。
 * 1回のドローコールで全トレイルを描画可能。
 */
class BulletTrailManager
{
public:
    /**
     * @brief シングルトンインスタンス取得
     * @return BulletTrailManagerの参照
     */
    static BulletTrailManager& GetInstance();

    /**
     * @brief 初期化
     * 
     * JSONからエフェクト定義を読み込み、MultiSourceRibbonModuleを取得。
     */
    void Initialize();

    /**
     * @brief 弾を登録
     * 
     * 弾丸のTransformをMultiSourceRibbonModuleに登録。
     * 
     * @param bulletTransform 追従対象のTransform
     * @return トレイルID（解除時に必要）= ribbonId
     */
    uint32_t RegisterBullet(Transform* bulletTransform);

    /**
     * @brief 弾を登録（手動更新版）
     * 
     * Transformポインタを使わず、システムから座標を直接送る場合に代わりに使用。
     * 
     * @return トレイルID（更新・解除時に必要）
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
    BulletTrailManager() = default;
    ~BulletTrailManager() = default;
    BulletTrailManager(const BulletTrailManager&) = delete;
    BulletTrailManager& operator=(const BulletTrailManager&) = delete;

private:
    // エフェクト定義名
    static constexpr const char* kEffectName = "BulletTrail";
    // JSONファイルパス
    static constexpr const char* kEffectJsonPath = "./Resources/json/particle/bulletTrail.json";

    // エフェクト（単一インスタンス、自動削除されないように保持）
    ParticleEffect* effect_ = nullptr;
    // MultiSourceRibbonModuleへの参照
    MultiSourceRibbonModule* multiSourceModule_ = nullptr;
    // 初期化済みフラグ
    bool initialized_ = false;
};
