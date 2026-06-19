#include "DecoySystem.h"
#include "engine/ecs/Registry.h"
#include "engine/ecs/components/LifetimeComponent.h"
#include "application/ecs/components/DecoyComponent.h"
#include "engine/effects/particle/ParticleEffect.h"
#include "engine/time/TimeManager.h"

using namespace ecs;


void DecoySystem::Update(Registry& registry)
{
    auto view = registry.View<DecoyComponent>();
    if (!view)
    {
        return;
    }

    float dt = TimeManager::GetInstance().GetGameContext().deltaTime;

    for (uint32_t i = 0; i < view->GetSize(); ++i)
    {
        EntityID entity = view->GetEntityFromDenseIndex(i);
        auto& decoy = view->GetDataFromDenseIndex(i);

        if (registry.HasComponent<LifetimeComponent>(entity))
        {
            auto& lifetime = registry.GetComponent<LifetimeComponent>(entity);
            
            // 消滅直前にエフェクトを停止
            if (lifetime.currentAge_ + dt >= lifetime.maxLifetime_)
            {
                if (decoy.effect_)
                {
                    decoy.effect_->Stop();
                    decoy.effect_->SetAutoRemove(true);
                    decoy.effect_ = nullptr;
                }
            }
        }
    }
}

