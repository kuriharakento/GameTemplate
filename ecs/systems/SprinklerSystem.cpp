#include "SprinklerSystem.h"
#include "engine/ecs/Registry.h"
#include "engine/ecs/components/TransformComponent.h"
#include "engine/ecs/components/TagComponent.h"
#include "engine/time/TimeManager.h"
#include "engine/effects/particle/ParticleManager.h"
#include "application/ecs/components/SprinklerComponent.h"
#include "application/ecs/components/StatusComponent.h"
#include "application/ecs/components/InducedExplosionComponent.h"
#include "audio/Audio.h"

using namespace ecs;


void SprinklerSystem::Update(Registry& registry)
{
    auto view = registry.View<SprinklerComponent>();
    if (!view)
    {
        return;
    }

    float dt = TimeManager::GetInstance().GetGameContext().deltaTime;

    for (uint32_t i = 0; i < view->GetSize(); ++i)
    {
        EntityID sprinklerEntity = view->GetEntityFromDenseIndex(i);
        auto& sprinkler = view->GetDataFromDenseIndex(i);

        if (!registry.HasComponent<TransformComponent>(sprinklerEntity))
        {
            continue;
        }
        auto& sprinklerTrans = registry.GetComponent<TransformComponent>(sprinklerEntity);

        sprinkler.checkTimer_ -= dt;
        if (sprinkler.checkTimer_ > 0.0f)
        {
            continue;
        }
        sprinkler.checkTimer_ = sprinkler.checkInterval_;

        float rangeSq = sprinkler.range_ * sprinkler.range_;

        auto tagView = registry.View<ecs::TagComponent>();
        if (!tagView)
        {
            continue;
        }

        for (uint32_t j = 0; j < tagView->GetSize(); ++j)
        {
            if (tagView->GetDataFromDenseIndex(j).type != ecs::TagComponent::Type::Enemy)
            {
                continue;
            }

            EntityID enemy = tagView->GetEntityFromDenseIndex(j);
            if (!registry.HasComponent<TransformComponent>(enemy))
            {
                continue;
            }
            if (!registry.HasComponent<ecs::InducedExplosionComponent>(enemy))
            {
                continue;
            }

            auto& enemyTrans = registry.GetComponent<TransformComponent>(enemy);
            float distSq = (enemyTrans.localPosition_ - sprinklerTrans.localPosition_).LengthSquared();
            if (distSq > rangeSq)
            {
                continue;
            }

            auto& stack = registry.GetComponent<ecs::InducedExplosionComponent>(enemy);
            if (stack.count_ <= 0)
            {
                continue;
            }

            Vector3 expPos = enemyTrans.localPosition_;

            if (registry.HasComponent<ecs::StatusComponent>(enemy))
            {
                auto& status = registry.GetComponent<ecs::StatusComponent>(enemy);
                static constexpr float kSprinklerDetonateDamage = 300.0f;
                status.hp_.SetBase(status.hp_.GetBase() - kSprinklerDetonateDamage);
                if (status.hp_.GetBase() <= 0.0f)
                {
                    status.isAlive_ = false;
                }
            }

            ParticleManager::GetInstance()->Play("E_explosion", expPos);
            Audio::GetInstance()->PlayWave("explosion", false);

            registry.RemoveComponent<ecs::InducedExplosionComponent>(enemy);
        }
    }
}

