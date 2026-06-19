#pragma once
#include "engine/ecs/system/ISystem.h"
#include "engine/ecs/Entity.h"
#include <cstdint>

class LevelUpUI;
class PostProcessManager;
class SkillSelectionUI;

/**
 * @brief 経験値の獲得とレベルアップを管理する。
 */
class ProgressionSystem : public ISystem
{
public:
    void Update(Registry& registry) override;

    void SetLevelUpUI(LevelUpUI* ui) { levelUpUI_ = ui; }
    void SetPostProcessManager(PostProcessManager* ppm) { postProcessManager_ = ppm; }
    void SetSkillSelectionUI(SkillSelectionUI* ui) { skillSelectionUI_ = ui; }

    bool IsPendingSkillSelection() const { return pendingSkillSelection_; }
    uint32_t GetPendingSelectionLevel() const { return pendingSelectionLevel_; }
    void ClearPendingSkillSelection() { pendingSkillSelection_ = false; pendingSelectionLevel_ = 0; }

private:
    /**
     * @brief レベルアップ報酬の適用。
     */
    void ApplyLevelUpRewards(EntityID entity, uint32_t newLevel, Registry& registry);

    /**
     * @brief レベルアップ演出の再生。
     */
    void PlayLevelUpEffects(EntityID entity, Registry& registry);

    LevelUpUI* levelUpUI_ = nullptr;
    PostProcessManager* postProcessManager_ = nullptr;
    SkillSelectionUI* skillSelectionUI_ = nullptr;

    bool pendingSkillSelection_ = false;
    uint32_t pendingSelectionLevel_ = 0;

    bool isVignetteActive_ = false;
    float vignetteTimer_ = 0.0f;
    static constexpr float kVignetteDuration = 1.2f;
};
