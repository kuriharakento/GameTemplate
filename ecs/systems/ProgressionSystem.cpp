#include "ProgressionSystem.h"
#include "engine/ecs/Registry.h"
#include "application/ecs/components/PlayerProgressionComponent.h"
#include "application/ecs/components/SkillComponent.h"
#include "application/ecs/components/StatusComponent.h"
#include "engine/ecs/components/TransformComponent.h"
#include "engine/effects/particle/ParticleManager.h"
#include "engine/manager/effect/PostProcessManager.h"
#include "engine/time/TimeManager.h"
#include "engine/math/Easing.h"
#include "application/UI/LevelUpUI.h"

using namespace ecs;


void ProgressionSystem::Update(Registry& registry)
{
    auto view = registry.View<PlayerProgressionComponent>();
    if (!view)
    {
        return;
    }

    for (uint32_t i = 0; i < view->GetSize(); ++i)
    {
        EntityID entity = view->GetEntityFromDenseIndex(i);
        auto& prog = view->GetDataFromDenseIndex(i);
        
        bool leveledUp = false;
        while (prog.currentExp_ >= prog.nextLevelExp_)
        {
            prog.currentExp_ -= prog.nextLevelExp_;
            prog.level_++;
            
            if (prog.level_ == 2)
            {
                prog.nextLevelExp_ = 15.0f;
            }
            else if (prog.level_ == 3)
            {
                prog.nextLevelExp_ = 25.0f;
            }
            else if (prog.level_ == 4)
            {
                prog.nextLevelExp_ = 50.0f;
            }
            else if (prog.level_ == 5)
            {
                prog.nextLevelExp_ = 80.0f;
            }
            else
            {
                prog.nextLevelExp_ *= 1.5f;
            }

            ApplyLevelUpRewards(entity, prog.level_, registry);
            leveledUp = true;

            if (pendingSkillSelection_)
            {
                break;
            }
        }

        if (leveledUp)
        {
            PlayLevelUpEffects(entity, registry);
        }
    }

    if (isVignetteActive_ && postProcessManager_ && postProcessManager_->vignetteEffect_)
    {
        float dt = TimeManager::GetInstance().GetGameContext().deltaTime;
        vignetteTimer_ += dt;

        float progress = std::clamp(vignetteTimer_ / kVignetteDuration, 0.0f, 1.0f);
        float intensity = 0.8f * (1.0f - EaseOutQuad(progress));

        auto& vignette = postProcessManager_->vignetteEffect_;
        vignette->SetEnabled(true);
        vignette->SetColor({ 1.0f, 1.0f, 0.0f });
        vignette->SetIntensity(intensity);

        if (progress >= 1.0f)
        {
            isVignetteActive_ = false;
            vignette->SetEnabled(false);
            vignette->SetColor({ 0.0f, 0.0f, 0.0f });
        }
    }
}

void ProgressionSystem::ApplyLevelUpRewards(EntityID entity, uint32_t newLevel, Registry& registry)
{
    if (registry.HasComponent<SkillComponent>(entity))
    {
        auto& skill = registry.GetComponent<SkillComponent>(entity);

        if (newLevel == 2 || newLevel == 3)
        {
            pendingSkillSelection_ = true;
            pendingSelectionLevel_ = newLevel;
        }
        else if (newLevel >= 4)
        {
            skill.isBeamUnlocked_ = true;
            pendingSkillSelection_ = true;
            pendingSelectionLevel_ = newLevel;
        }
    }
}

void ProgressionSystem::PlayLevelUpEffects(EntityID entity, Registry& registry)
{
    if (registry.HasComponent<TransformComponent>(entity))
    {
        auto& trans = registry.GetComponent<TransformComponent>(entity);
        ParticleManager::GetInstance()->Play("level_up", trans.localPosition_);
    }

    if (postProcessManager_ && postProcessManager_->vignetteEffect_)
    {
        isVignetteActive_ = true;
        vignetteTimer_ = 0.0f;
        
        postProcessManager_->vignetteEffect_->SetEnabled(true);
        postProcessManager_->vignetteEffect_->SetColor({ 1.0f, 1.0f, 0.0f });
        postProcessManager_->vignetteEffect_->SetIntensity(0.8f);
    }

    if (levelUpUI_)
    {
        levelUpUI_->Trigger();
    }
}

