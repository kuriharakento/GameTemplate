#include "ProjectileSystem.h"
#include <cmath>
#include "engine/ecs/Registry.h"
#include "engine/ecs/components/TransformComponent.h"
#include "engine/ecs/components/TagComponent.h"
#include "application/ecs/components/ProjectileComponent.h"
#include "engine/time/TimeManager.h"
#include "application/effect/BulletTrailManager.h"
#include "application/effect/HomingTrailManager.h"

using namespace ecs;


void ProjectileSystem::Update(Registry& registry)
{
    float dt = TimeManager::GetInstance().GetGameContext().deltaTime;

    auto view = registry.View<ProjectileComponent>();
    if (!view)
    {
        return;
    }

    for (uint32_t i = 0; i < view->GetSize(); ++i)
    {
        EntityID entity = view->GetEntityFromDenseIndex(i);
        if (!registry.HasComponent<TransformComponent>(entity))
        {
            continue;
        }

        ProjectileComponent& pc = view->GetDataFromDenseIndex(i);
        TransformComponent& trans = registry.GetComponent<TransformComponent>(entity);

        // ホーミング
        if (pc.isHoming_ && pc.targetEntity_ != kInvalidEntity && registry.IsAlive(pc.targetEntity_))
        {
            if (registry.HasComponent<TransformComponent>(pc.targetEntity_))
            {
                const auto& targetTrans = registry.GetComponent<TransformComponent>(pc.targetEntity_);
                Vector3 toTarget = (targetTrans.localPosition_ - trans.localPosition_).Normalize();

                static constexpr float kTurnSpeed = 40.0f;
                pc.velocity_ = (pc.velocity_ + toTarget * (pc.speed_ * kTurnSpeed * dt)).Normalize() * pc.speed_;

                float yaw = std::atan2(pc.velocity_.x, pc.velocity_.z);
                trans.localRotation_.y = yaw;
            }
        }

        trans.localPosition_ = trans.localPosition_ + pc.velocity_ * dt;
        trans.isDirty_ = true;

        // トレイル更新
        if (pc.trailId_ > 0)
        {
            if (pc.trailType_ == ProjectileComponent::TrailType::Bullet)
            {
                BulletTrailManager::GetInstance().UpdateBulletManual(pc.trailId_, trans.localPosition_);
            }
            else if (pc.trailType_ == ProjectileComponent::TrailType::Homing)
            {
                HomingTrailManager::GetInstance().UpdateBulletManual(pc.trailId_, trans.localPosition_);
            }
        }

        // 寿命
        pc.lifetime_ -= dt;
        if (pc.lifetime_ <= 0.0f)
        {
            if (pc.trailId_ > 0)
            {
                if (pc.trailType_ == ProjectileComponent::TrailType::Bullet)
                {
                    BulletTrailManager::GetInstance().UnregisterBullet(pc.trailId_);
                }
                else if (pc.trailType_ == ProjectileComponent::TrailType::Homing)
                {
                    HomingTrailManager::GetInstance().UnregisterBullet(pc.trailId_);
                }
                pc.trailId_ = 0;
            }
            registry.DestroyEntityDeferred(entity);
        }
    }
}

