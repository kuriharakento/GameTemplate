#include "TurretSystem.h"
#include "engine/ecs/Registry.h"
#include "engine/ecs/components/TransformComponent.h"
#include "engine/ecs/components/TagComponent.h"
#include "engine/ecs/system/SystemManager.h"
#include "engine/ecs/system/CollisionSystem.h"
#include "engine/time/TimeManager.h"
#include "engine/effects/particle/ParticleManager.h"
#include "application/ecs/components/TurretComponent.h"
#include "application/ecs/components/SkillComponent.h"
#include "application/ecs/components/StatusComponent.h"
#include "application/ecs/components/DamageStackComponent.h"
#include "application/ecs/components/InducedExplosionComponent.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include "application/ecs/components/ProjectileComponent.h"
#include "application/effect/BulletTrailManager.h"
#include "application/effect/HomingTrailManager.h"
#include "application/ecs/CollisionConfig.h"
#include "engine/ecs/components/ColliderComponent.h"
#include "engine/ecs/components/CollisionResponseComponent.h"
#include "engine/audio/Audio.h"
#include "engine/math/MathUtils.h"
#include "engine/ecs/components/InstancedRenderComponent.h"
#include "engine/effects/particle/ParticleEffect.h"
#include "engine/effects/particle/module/spawn/InitialModules.h"
#include "engine/effects/particle/module/spawn/SpawnShapeModules.h"

using namespace ecs;


void TurretSystem::Update(Registry& registry)
{
    auto view = registry.View<TurretComponent>();
    if (!view)
    {
        return;
    }

    float dt = TimeManager::GetInstance().GetGameContext().deltaTime;

    for (uint32_t i = 0; i < view->GetSize(); ++i)
    {
        EntityID turretEntity = view->GetEntityFromDenseIndex(i);
        auto& turret = view->GetDataFromDenseIndex(i);

        if (!registry.HasComponent<TransformComponent>(turretEntity))
        {
            continue;
        }
        auto& turretTrans = registry.GetComponent<TransformComponent>(turretEntity);

        turret.fireTimer_ -= dt;

        SkillSpecialChoice specialType = SkillSpecialChoice::None;
        bool isBuffActive = false;
        float qRateMult = 1.0f;
        float eSalvoDmgMult = 1.0f;

        if (turret.owner_ != kInvalidEntity && registry.HasComponent<SkillComponent>(turret.owner_))
        {
            auto& skill = registry.GetComponent<SkillComponent>(turret.owner_);
            specialType = skill.special_;
            isBuffActive = skill.isTurretBuffActive_;
            qRateMult = skill.qTurretFireRateMult_;
            eSalvoDmgMult = skill.eSalvoDamageMult_;
        }

        if (isBuffActive && specialType == SkillSpecialChoice::PlasmaLaser)
        {
            UpdateLaserBeam(turretEntity, turret, registry, dt);
            continue;
        }
        else
        {
            if (turret.activeBeam_ != kInvalidEntity)
            {
                registry.DestroyEntityDeferred(turret.activeBeam_);
                turret.activeBeam_ = kInvalidEntity;
            }
            if (turret.laserEffect_)
            {
                turret.laserEffect_->Stop();
                turret.laserEffect_->SetAutoRemove(true);
                turret.laserEffect_ = nullptr;
            }
        }

        EntityID closestEnemy = kInvalidEntity;
        float closestDistSq = turret.range_ * turret.range_;

        auto tagView = registry.View<ecs::TagComponent>();
        if (tagView)
        {
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

                auto& enemyTrans = registry.GetComponent<TransformComponent>(enemy);
                float distSq = (enemyTrans.localPosition_ - turretTrans.localPosition_).LengthSquared();
                if (distSq < closestDistSq)
                {
                    closestDistSq = distSq;
                    closestEnemy = enemy;
                }
            }
        }

        if (closestEnemy != kInvalidEntity)
        {
            Vector3 enemyPos = registry.GetComponent<TransformComponent>(closestEnemy).localPosition_;
            Vector3 startPos = turretTrans.localPosition_;
            Vector3 direction = (enemyPos - startPos).Normalize();

            float targetYaw = std::atan2(direction.x, direction.z);
            turretTrans.localRotation_.y = MathUtils::LerpAngle(turretTrans.localRotation_.y, targetYaw, 0.2f);
            turretTrans.isDirty_ = true;
        }

        if (turret.fireTimer_ > 0.0f)
        {
            continue;
        }
        if (closestEnemy == kInvalidEntity)
        {
            continue;
        }

        turret.fireTimer_ = turret.fireInterval_ * qRateMult;

        EntityID proj = registry.CreateEntity();

        Vector3 spawnStartPos = turretTrans.localPosition_;
        spawnStartPos.y += 0.5f;

        Vector3 enemyTargetPos = registry.GetComponent<TransformComponent>(closestEnemy).localPosition_;
        enemyTargetPos.y += 0.5f;
        Vector3 shotDir = (enemyTargetPos - spawnStartPos).Normalize();

        registry.AddComponent<TransformComponent>(proj, { spawnStartPos, turretTrans.localRotation_, { 1.0f, 1.0f, 1.0f } });

        ProjectileComponent pc;
        pc.type_ = ProjectileComponent::Type::Lmb; 
        pc.speed_ = 80.0f; 
        pc.velocity_ = shotDir * pc.speed_;
        pc.lifetime_ = 1.5f;

        if (isBuffActive && specialType == SkillSpecialChoice::MissileSalvo)
        {
            pc.speed_ = 40.0f;
            pc.velocity_ = shotDir * pc.speed_;
            pc.damage_ = turret.damage_ * eSalvoDmgMult;
            pc.isHoming_ = true;
            pc.targetEntity_ = closestEnemy;
            pc.trailType_ = ProjectileComponent::TrailType::Homing;
            pc.trailId_ = HomingTrailManager::GetInstance().RegisterBulletManual();
            HomingTrailManager::GetInstance().UpdateBulletManual(pc.trailId_, spawnStartPos);
        }
        else
        {
            pc.damage_ = turret.damage_;
            pc.isHoming_ = false;
            pc.trailType_ = ProjectileComponent::TrailType::Bullet;
            pc.trailId_ = BulletTrailManager::GetInstance().RegisterBulletManual();
            BulletTrailManager::GetInstance().UpdateBulletManual(pc.trailId_, spawnStartPos);
        }

        registry.AddComponent<ProjectileComponent>(proj, pc);

        ecs::ColliderComponent col;
        col.type_ = ColliderType::Sphere;
        col.sphere_.radius = 0.5f;
        col.isTrigger_ = true;
        col.layer = CollisionLayer::kPlayerBullet;
        col.mask = CollisionLayer::kEnemy | CollisionLayer::kObstacle;

        float shotDamage = pc.damage_;
        col.onCollisionEnter = [this, &registry, proj, shotDamage](const ecs::CollisionPartnerInfo& other)
        {
            if (!registry.IsAlive(other.entity))
            {
                return;
            }
            if (!registry.HasComponent<ecs::TagComponent>(other.entity))
            {
                return;
            }
            if (registry.GetComponent<ecs::TagComponent>(other.entity).type != ecs::TagComponent::Type::Enemy)
            {
                return;
            }

            if (registry.HasComponent<ecs::StatusComponent>(other.entity))
            {
                auto& status = registry.GetComponent<ecs::StatusComponent>(other.entity);
                status.hp_.SetBase(status.hp_.GetBase() - shotDamage);
            }
            Vector3 hitPos = registry.GetComponent<TransformComponent>(other.entity).localPosition_;
            ParticleManager::GetInstance()->Play("hit_effect_ver2", hitPos);
            registry.DestroyEntityDeferred(proj);
        };

        registry.AddComponent<ecs::ColliderComponent>(proj, col);
        registry.AddComponent<CollisionResponseComponent>(proj, {});

        Audio::GetInstance()->PlayWave("fire", false);
    }
}

void TurretSystem::UpdateLaserBeam(EntityID turretEntity, TurretComponent& turret, Registry& registry, float dt)
{
    (void)dt;
    auto& turretTrans = registry.GetComponent<TransformComponent>(turretEntity);

    EntityID closestEnemy = kInvalidEntity;
    float closestDistSq = turret.range_ * turret.range_;

    auto tagView = registry.View<ecs::TagComponent>();
    if (tagView)
    {
        for (uint32_t j = 0; j < tagView->GetSize(); j++)
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
            auto& enemyTrans = registry.GetComponent<TransformComponent>(enemy);
            float distSq = (enemyTrans.localPosition_ - turretTrans.localPosition_).LengthSquared();
            if (distSq < closestDistSq)
            {
                closestDistSq = distSq;
                closestEnemy = enemy;
            }
        }
    }

    if (closestEnemy == kInvalidEntity)
    {
        if (turret.activeBeam_ != kInvalidEntity)
        {
            registry.DestroyEntityDeferred(turret.activeBeam_);
            turret.activeBeam_ = kInvalidEntity;
        }
        if (turret.laserEffect_)
        {
            turret.laserEffect_->Stop();
            turret.laserEffect_->SetAutoRemove(true);
            turret.laserEffect_ = nullptr;
        }
        return;
    }

    Vector3 targetPos = registry.GetComponent<TransformComponent>(closestEnemy).localPosition_;
    targetPos.y += 0.5f;
    Vector3 startPos = turretTrans.localPosition_;
    startPos.y += 0.5f;
    Vector3 direction = (targetPos - startPos).Normalize();

    float targetYaw = std::atan2(direction.x, direction.z);
    turretTrans.localRotation_.y = MathUtils::LerpAngle(turretTrans.localRotation_.y, targetYaw, 0.2f);
    turretTrans.isDirty_ = true;

    if (turret.activeBeam_ == kInvalidEntity || !registry.IsAlive(turret.activeBeam_))
    {
        turret.activeBeam_ = registry.CreateEntity();
        
        ecs::ColliderComponent col;
        col.type_ = ColliderType::OBB;
        col.obb_.size = { 0.5f, 0.5f, 0.5f };
        col.isTrigger_ = true;
        col.layer = CollisionLayer::kPlayerBullet;
        col.mask = CollisionLayer::kEnemy;

        float dps = turret.damage_ * 30.0f;
        col.onCollisionStay = [&registry, dps](const ecs::CollisionPartnerInfo& other)
        {
            if (!registry.IsAlive(other.entity))
            {
                return;
            }
            if (!registry.HasComponent<ecs::StatusComponent>(other.entity))
            {
                return;
            }
            
            auto& status = registry.GetComponent<ecs::StatusComponent>(other.entity);
            float frameDmg = dps * TimeManager::GetInstance().GetGameContext().deltaTime;
            status.hp_.SetBase(status.hp_.GetBase() - frameDmg);

            static float effectTimer = 0.0f;
            effectTimer += TimeManager::GetInstance().GetGameContext().deltaTime;
            if (effectTimer > 0.1f)
            {
                Vector3 hitPos = registry.GetComponent<TransformComponent>(other.entity).localPosition_;
                ParticleManager::GetInstance()->Play("hit_effect_ver2", hitPos);
                effectTimer = 0.0f;
            }
        };

        registry.AddComponent<ecs::ColliderComponent>(turret.activeBeam_, col);
        registry.AddComponent<CollisionResponseComponent>(turret.activeBeam_, {});
    }

    if (!registry.HasComponent<TransformComponent>(turret.activeBeam_))
    {
        registry.AddComponent<TransformComponent>(turret.activeBeam_, { (startPos + targetPos) * 0.5f, turretTrans.localRotation_, { 0.3f, 0.3f, std::sqrt(closestDistSq) } });
    }

    auto& beamTrans = registry.GetComponent<TransformComponent>(turret.activeBeam_);
    beamTrans.localPosition_ = (startPos + targetPos) * 0.5f;
    beamTrans.localRotation_ = turretTrans.localRotation_; 
    float dist = std::sqrt(closestDistSq);
    beamTrans.localScale_ = { 0.3f, 0.3f, dist }; 
    beamTrans.isDirty_ = true;

    if (!turret.laserEffect_ || !turret.laserEffect_->IsPlaying())
    {
        turret.laserEffect_ = ParticleManager::GetInstance()->Play("turret_lazer", startPos);
        if (turret.laserEffect_)
        {
            turret.laserEffect_->SetAutoRemove(false);
        }
    }

    if (turret.laserEffect_)
    {
        turret.laserEffect_->SetPosition(beamTrans.localPosition_);
        float yawDeg = targetYaw * (180.0f / 3.14159265f);
        
        for (uint32_t j = 0; j < turret.laserEffect_->GetEmitterCount(); j++)
        {
            auto* emitter = turret.laserEffect_->GetEmitter(j);
            if (!emitter)
            {
                continue;
            }

            if (auto* rotModule = emitter->GetModule<InitialRotationModule>())
            {
                rotModule->SetRotationRange({ 90.0f, yawDeg, 0 }, { 90.0f, yawDeg, 0 });
            }

            if (auto* scaleModule = emitter->GetModule<InitialScaleModule>())
            {
                Vector3 minS = scaleModule->GetMinScale();
                Vector3 maxS = scaleModule->GetMaxScale();
                minS.y = beamTrans.localScale_.z * 0.5f;
                maxS.y = beamTrans.localScale_.z * 0.5f;
                scaleModule->SetScaleRange(minS, maxS);
            }
        }
    }
}

