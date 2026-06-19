#include "PlayerActionSystem.h"
#include <algorithm>

#include "engine/ecs/Registry.h"
#include "engine/ecs/components/TransformComponent.h"
#include "engine/ecs/components/TagComponent.h"
#include "engine/ecs/components/ColliderComponent.h"
#include "engine/ecs/components/CollisionResponseComponent.h"
#include "engine/ecs/components/InstancedRenderComponent.h"
#include "engine/time/TimeManager.h"
#include "engine/ecs/system/SystemManager.h"
#include "engine/ecs/system/CollisionSystem.h"
#include "engine/ecs/components/LifetimeComponent.h"
#include "engine/ecs/components/HierarchyComponent.h"
#include "engine/manager/graphics/LineManager.h"
#include "math/VectorColorCodes.h"
#include "engine/effects/particle/ParticleManager.h"
#include "engine/effects/particle/module/spawn/InitialModules.h"

#include "application/ecs/components/PlayerProgressionComponent.h"
#include "application/ecs/components/SkillComponent.h"
#include "engine/ecs/components/Object3dComponent.h"
#include "application/ecs/components/ProjectileComponent.h"
#include "application/ecs/components/InducedExplosionComponent.h"
#include "application/ecs/CollisionConfig.h"
#include "application/effect/BulletTrailManager.h"
#include "application/effect/HomingTrailManager.h"
#include "application/ecs/components/DodgeComponent.h"
#include "application/ecs/components/StatusComponent.h"
#include "application/ecs/components/TurretComponent.h"
#include "application/ecs/components/SprinklerComponent.h"
#include "application/ecs/components/DamageStackComponent.h"
#include "application/ecs/components/DecoyComponent.h"

#include "input/Input.h"
#include "math/MathUtils.h"
#include "math/MatrixFunc.h"
#include "manager/scene/CameraManager.h"
#include "base/Camera.h"
#include "base/WinApp.h"
#include "engine/effects/particle/module/spawn/SpawnShapeModules.h"
#include "engine/effects/particle/ParticleEffect.h"
#include "audio/Audio.h"

using namespace ecs;


void PlayerActionSystem::Update(Registry& registry)
{
    auto tagView = registry.View<ecs::TagComponent>();
    if (!tagView)
    {
        return;
    }

    float dt = TimeManager::GetInstance().GetGameContext().deltaTime;

    for (uint32_t i = 0; i < tagView->GetSize(); ++i)
    {
        if (tagView->GetDataFromDenseIndex(i).type != ecs::TagComponent::Type::Player)
        {
            continue;
        }

        EntityID entity = tagView->GetEntityFromDenseIndex(i);

        if (!registry.HasComponent<DodgeComponent>(entity))
        {
            continue;
        }

        UpdateDodge(entity, registry, dt);

        auto& dodge = registry.GetComponent<DodgeComponent>(entity);
        if (!dodge.isDodging_)
        {
            if (registry.HasComponent<TransformComponent>(entity) &&
                registry.HasComponent<ecs::StatusComponent>(entity))
            {
                UpdateMovement(entity, registry, dt);
            }

            if (registry.HasComponent<SkillComponent>(entity))
            {
                UpdateSkills(entity, registry, dt);

                auto& skill = registry.GetComponent<SkillComponent>(entity);
                if (skill.activeBeamParticle_)
                {
                    if (skill.beamActiveTimer_ > 0.0f)
                    {
                        skill.beamActiveTimer_ -= dt;
                        if (registry.HasComponent<TransformComponent>(entity))
                        {
                            auto& trans = registry.GetComponent<TransformComponent>(entity);

                            skill.activeBeamParticle_->SetPosition(trans.localPosition_);

                            float yaw = trans.localRotation_.y;
                            Vector3 forward = { std::sin(yaw), 0.0f, std::cos(yaw) };

                            if (auto* e2 = skill.activeBeamParticle_->GetEmitter("e2"))
                            {
                                e2->SetFollowOffset(forward * 60.0f);

                                if (auto* rot = e2->GetModule<InitialRotationModule>())
                                {
                                    float radToDeg = 180.0f / 3.14159265f;
                                    Vector3 rotDeg = { -90.0f, yaw * radToDeg, 0.0f };
                                    rot->SetRotationRange(rotDeg, rotDeg);
                                }
                            }
                            if (auto* e3 = skill.activeBeamParticle_->GetEmitter("e3"))
                            {
                                e3->SetFollowOffset(forward * 50.0f);

                                if (auto* rot = e3->GetModule<InitialRotationModule>())
                                {
                                    float radToDeg = 180.0f / 3.14159265f;
                                    Vector3 rotDeg = { -90.0f, yaw * radToDeg, 0.0f };
                                    rot->SetRotationRange(rotDeg, rotDeg);
                                }
                            }
                        }

                        if (skill.beamActiveTimer_ <= 0.0f)
                        {
                            skill.activeBeamParticle_->Stop();
                            skill.activeBeamParticle_ = nullptr;
                        }
                    }
                    else
                    {
                        skill.activeBeamParticle_->Stop();
                        skill.activeBeamParticle_ = nullptr;
                    }
                }
            }
        }
    }
}

void PlayerActionSystem::UpdateDodge(EntityID entity, Registry& registry, float dt)
{
    auto& dodge = registry.GetComponent<DodgeComponent>(entity);
    auto& trans = registry.GetComponent<TransformComponent>(entity);

    if (dodge.cooldownTimer_ > 0.0f)
    {
        dodge.cooldownTimer_ -= dt;
    }

    if (dodge.isDodging_)
    {
        dodge.timer_ -= dt;
        float progress = 1.0f - ((std::max)(0.0f, dodge.timer_) / DodgeComponent::kDefaultDuration);

        trans.localPosition_ = MathUtils::Lerp(dodge.startPosition_, dodge.targetPosition_, progress);
        trans.isDirty_ = true;

        if (dodge.timer_ <= 0.0f)
        {
            dodge.isDodging_ = false;
            dodge.cooldownTimer_ = DodgeComponent::kDefaultCooldown;
        }
    }
    else
    {
        if (Input::GetInstance()->TriggerKey(DIK_SPACE) && dodge.cooldownTimer_ <= 0.0f)
        {
            auto* input = Input::GetInstance();
            Vector3 inputDir = { 0, 0, 0 };
            if (input->PushKey(DIK_W)) inputDir.z += 1.0f;
            if (input->PushKey(DIK_S)) inputDir.z -= 1.0f;
            if (input->PushKey(DIK_A)) inputDir.x -= 1.0f;
            if (input->PushKey(DIK_D)) inputDir.x += 1.0f;

            if (inputDir.LengthSquared() > 0.01f)
            {
                if (cameraManager_)
                {
                    float yaw = cameraManager_->GetActiveCamera()->GetRotate().y;
                    Vector3 forward(sin(yaw), 0, cos(yaw));
                    Vector3 right(cos(yaw), 0, -sin(yaw));
                    dodge.direction_ = (forward * inputDir.z + right * inputDir.x).Normalize();
                }
                else
                {
                    dodge.direction_ = inputDir.Normalize();
                }
            }
            else
            {
                float yaw = trans.localRotation_.y;
                dodge.direction_ = { sin(yaw), 0, cos(yaw) };
            }

            dodge.isDodging_ = true;
            dodge.timer_ = DodgeComponent::kDefaultDuration;
            dodge.startPosition_ = trans.localPosition_;
            dodge.targetPosition_ = dodge.startPosition_ + dodge.direction_ * DodgeComponent::kDefaultDistance;
        }
    }
}

void PlayerActionSystem::UpdateMovement(EntityID entity, Registry& registry, float dt)
{
    auto* input = Input::GetInstance();
    auto& trans = registry.GetComponent<TransformComponent>(entity);
    auto& status = registry.GetComponent<ecs::StatusComponent>(entity);

    Vector3 moveInput = { 0, 0, 0 };
    if (input->PushKey(DIK_W)) moveInput.z += 1.0f;
    if (input->PushKey(DIK_S)) moveInput.z -= 1.0f;
    if (input->PushKey(DIK_A)) moveInput.x -= 1.0f;
    if (input->PushKey(DIK_D)) moveInput.x += 1.0f;

    if (moveInput.LengthSquared() > 0.01f)
    {
        Vector3 finalDir = moveInput.Normalize();
        if (cameraManager_)
        {
            float yaw = cameraManager_->GetActiveCamera()->GetRotate().y;
            Vector3 forward(sin(yaw), 0, cos(yaw));
            Vector3 right(cos(yaw), 0, -sin(yaw));
            finalDir = (forward * moveInput.z + right * moveInput.x).Normalize();
        }

        float speed = status.moveSpeed_.GetValue();
        trans.localPosition_ = trans.localPosition_ + finalDir * speed * dt;
        trans.isDirty_ = true;
    }

    if (cameraManager_)
    {
        Camera* camera = cameraManager_->GetActiveCamera();
        float mouseX = input->GetMouseX();
        float mouseY = input->GetMouseY();

        // 1280x720の仮想スクリーン解像度に固定
        Matrix4x4 matViewport = MakeViewportMatrix(0, 0, 1280.0f, 720.0f, 0.0f, 1.0f);
        Matrix4x4 matVPV = (camera->GetViewMatrix() * camera->GetProjectionMatrix()) * matViewport;
        Matrix4x4 matInverseVPV = Inverse(matVPV);

        Vector3 posNear = MathUtils::Transform({ mouseX, mouseY, 0.0f }, matInverseVPV);
        Vector3 posFar = MathUtils::Transform({ mouseX, mouseY, 1.0f }, matInverseVPV);

        Vector3 rayDir = (posFar - posNear).Normalize();
        if (std::abs(rayDir.y) > 0.0001f)
        {
            float t = (trans.localPosition_.y - posNear.y) / rayDir.y;
            Vector3 targetPos = posNear + rayDir * t;
            Vector3 lookDir = (targetPos - trans.localPosition_);
            lookDir.y = 0.0f;

            if (lookDir.LengthSquared() > 0.01f)
            {
                float targetYaw = atan2f(lookDir.x, lookDir.z);
                trans.localRotation_.y = MathUtils::LerpAngle(trans.localRotation_.y, targetYaw, 0.2f);
                trans.isDirty_ = true;
            }
        }
    }
}

void PlayerActionSystem::UpdateSkills(EntityID entity, Registry& registry, float dt)
{
    auto& skill = registry.GetComponent<SkillComponent>(entity);

    if (skill.lmbTimer_ > 0.0f)
    {
        skill.lmbTimer_ -= dt;
    }
    if (skill.baseSkillTimer_ > 0.0f)
    {
        skill.baseSkillTimer_ -= dt;
    }
    if (skill.specialSkillTimer_ > 0.0f)
    {
        skill.specialSkillTimer_ -= dt;
    }

    if (skill.isTurretBuffActive_)
    {
        skill.turretBuffTimer_ -= dt;
        if (skill.turretBuffTimer_ <= 0.0f)
        {
            skill.isTurretBuffActive_ = false;
        }
    }

    UpdateLMB(entity, registry, dt);

    if (skill.route_ != SkillRoute::None)
    {
        UpdateSkill1(entity, registry, dt);
    }

    if (skill.special_ != SkillSpecialChoice::None)
    {
        UpdateSkill2(entity, registry, dt);
    }

    UpdateR(entity, registry, dt);
}

void PlayerActionSystem::UpdateLMB(EntityID entity, Registry& registry, float)
{
    auto& skill = registry.GetComponent<SkillComponent>(entity);
    if (!skill.isLmbUnlocked_)
    {
        return;
    }
    if (skill.lmbTimer_ > 0.0f)
    {
        return;
    }

    if (Input::GetInstance()->IsMouseButtonPressed(0))
    {
        Audio::GetInstance()->PlayWave("fire", false);
        auto& trans = registry.GetComponent<TransformComponent>(entity);
        float baseYaw = trans.localRotation_.y;

        uint32_t bulletPool = skill.lmbBulletCount_;
        float spreadAngle = 0.15f;
        float startYaw = baseYaw - spreadAngle * (bulletPool - 1) * 0.5f;

        for (uint32_t bIdx = 0; bIdx < bulletPool; ++bIdx)
        {
            float curYaw = startYaw + spreadAngle * bIdx;
            Vector3 dir = { sin(curYaw), 0, cos(curYaw) };

            EntityID proj = registry.CreateEntity();
            Vector3 spawnPos = { trans.localPosition_.x, 0.5f, trans.localPosition_.z };
            registry.AddComponent<TransformComponent>(proj, { spawnPos, {0, curYaw, 0}, {1.5f, 1.5f, 1.5f} });

            ProjectileComponent pc;
            pc.type_ = ProjectileComponent::Type::Lmb;
            pc.velocity_ = dir * skill.lmbProjectileSpeed_;
            pc.damage_ = 80.0f * skill.lmbDamageMultiplier_;
            pc.lifetime_ = 1.5f;
            pc.pierceCount_ = skill.lmbPierceCount_;
            pc.trailId_ = BulletTrailManager::GetInstance().RegisterBulletManual();
            registry.AddComponent<ProjectileComponent>(proj, pc);

            InstancedRenderComponent render;
            render.modelName_ = "bullet";
            render.useInstancing_ = true;
            registry.AddComponent<InstancedRenderComponent>(proj, render);

            ecs::ColliderComponent col;
            col.type_ = ColliderType::Sphere;
            col.sphere_.radius = 0.4f;
            col.previousPosition_ = trans.localPosition_;
            col.isTrigger_ = true;
            col.layer = CollisionLayer::kPlayerBullet;
            col.mask = CollisionLayer::kEnemy | CollisionLayer::kObstacle;

            col.onCollisionEnter = [this, &registry, proj](const ecs::CollisionPartnerInfo& other)
            {
                if (!registry.IsAlive(proj))
                {
                    return;
                }

                if (registry.HasComponent<ecs::ColliderComponent>(other.entity))
                {
                    auto& otherCol = registry.GetComponent<ecs::ColliderComponent>(other.entity);
                    if (otherCol.layer & CollisionLayer::kEnemy)
                    {
                        if (registry.HasComponent<ecs::StatusComponent>(other.entity))
                        {
                            auto& status = registry.GetComponent<ecs::StatusComponent>(other.entity);
                            float bonusDamage = 0.0f;

                            if (registry.HasComponent<DamageStackComponent>(other.entity))
                            {
                                auto& dmgStack = registry.GetComponent<DamageStackComponent>(other.entity);
                                bonusDamage = dmgStack.count_ * DamageStackComponent::kDamagePerStack;
                                dmgStack.count_ = 0;
                            }

                            status.hp_.SetBase(status.hp_.GetBase() - (80.0f + bonusDamage));
                        }

                        if (registry.HasComponent<ecs::InducedExplosionComponent>(other.entity))
                        {
                            auto& stack = registry.GetComponent<ecs::InducedExplosionComponent>(other.entity);
                            stack.count_ += 3;
                            if (stack.count_ >= ecs::InducedExplosionComponent::kMaxCount)
                            {
                                SpawnExplosion(other.entity, registry);
                            }
                        }

                        if (registry.HasComponent<TransformComponent>(proj))
                        {
                            ParticleManager::GetInstance()->Play("hit_effect_ver2", registry.GetComponent<TransformComponent>(proj).localPosition_);
                        }

                        auto& pc = registry.GetComponent<ProjectileComponent>(proj);
                        if (pc.pierceCount_ > 0)
                        {
                            pc.pierceCount_--;
                        }
                        else
                        {
                            registry.DestroyEntityDeferred(proj);
                        }
                    }
                    else if (otherCol.layer & CollisionLayer::kObstacle)
                    {
                        registry.DestroyEntityDeferred(proj);
                    }
                }
            };

            registry.AddComponent<ecs::ColliderComponent>(proj, col);
            registry.AddComponent<CollisionResponseComponent>(proj, {});
        }

        skill.lmbTimer_ = SkillComponent::kLmbBaseCooldown * skill.lmbCooldownMultiplier_;
    }
}

void PlayerActionSystem::UpdateSkill1(EntityID entity, Registry& registry, float)
{
    auto& skill = registry.GetComponent<SkillComponent>(entity);
    if (skill.baseSkillTimer_ > 0.0f)
    {
        return;
    }

    if (Input::GetInstance()->TriggerKey(DIK_Q))
    {
        if (skill.route_ == SkillRoute::Bomb)
        {
            FireBombWave(entity, registry);

            for (auto it = skill.activeDecoyEntities_.begin(); it != skill.activeDecoyEntities_.end(); )
            {
                if (registry.IsAlive(*it))
                {
                    FireBombWave(*it, registry);
                    ++it;
                }
                else
                {
                    it = skill.activeDecoyEntities_.erase(it);
                }
            }
        }
        else if (skill.route_ == SkillRoute::Turret)
        {
            SpawnTurret(entity, registry);
        }

        skill.baseSkillTimer_ = SkillComponent::kBaseSkillCooldown * skill.qCooldownMultiplier_;
    }
}

void PlayerActionSystem::UpdateSkill2(EntityID entity, Registry& registry, float)
{
    auto& skill = registry.GetComponent<SkillComponent>(entity);
    if (skill.specialSkillTimer_ > 0.0f)
    {
        return;
    }

    if (Input::GetInstance()->TriggerKey(DIK_E))
    {
        switch (skill.special_)
        {
        case SkillSpecialChoice::HomingMissile:
            FireHomingMissile(entity, registry);
            break;
        case SkillSpecialChoice::DecoyBomb:
            SpawnDecoy(entity, registry);
            break;
        case SkillSpecialChoice::MissileSalvo:
        case SkillSpecialChoice::PlasmaLaser:
            skill.isTurretBuffActive_ = true;
            skill.turretBuffTimer_ = SkillComponent::kTurretBuffDuration;
            if (registry.HasComponent<TransformComponent>(entity))
            {
                ParticleManager::GetInstance()->Play("hit_effect_ver2", registry.GetComponent<TransformComponent>(entity).localPosition_);
            }
            break;
        default:
            break;
        }

        skill.specialSkillTimer_ = SkillComponent::kSpecialSkillCooldown * skill.eCooldownMultiplier_;
    }
}

void PlayerActionSystem::FireBombWave(EntityID entity, Registry& registry)
{
    if (!registry.HasComponent<TransformComponent>(entity))
    {
        return;
    }
    auto& trans = registry.GetComponent<TransformComponent>(entity);

    float range = 15.0f;
    auto skillView = registry.View<SkillComponent>();
    if (skillView && skillView->GetSize() > 0)
    {
        range = skillView->GetDataFromDenseIndex(0).qRange_;
    }

    EntityID wave = registry.CreateEntity();
    Vector3 wavePos = trans.localPosition_;
    wavePos.y = 0.5f;
    registry.AddComponent<TransformComponent>(wave, { wavePos, {0,0,0}, {1,1,1} });
    registry.AddComponent<LifetimeComponent>(wave, { 0.0f, 0.15f });

    ecs::ColliderComponent col;
    col.type_ = ColliderType::Sphere;
    col.sphere_.radius = range;
    col.isTrigger_ = true;
    col.layer = CollisionLayer::kPlayerBullet;
    col.mask = CollisionLayer::kEnemy;

    static constexpr float kWaveDamage = 20.0f;

    col.onCollisionEnter = [this, &registry](const ecs::CollisionPartnerInfo& other)
    {
        if (registry.HasComponent<ecs::TagComponent>(other.entity) &&
            registry.GetComponent<ecs::TagComponent>(other.entity).type == ecs::TagComponent::Type::Enemy)
        {
            if (!registry.HasComponent<ecs::InducedExplosionComponent>(other.entity))
            {
                registry.AddComponent<ecs::InducedExplosionComponent>(other.entity, { 1 });
            }
            else
            {
                auto& stack = registry.GetComponent<ecs::InducedExplosionComponent>(other.entity);
                stack.count_++;
                if (stack.count_ >= ecs::InducedExplosionComponent::kMaxCount)
                {
                    SpawnExplosion(other.entity, registry);
                }
            }

            if (registry.HasComponent<ecs::StatusComponent>(other.entity))
            {
                auto& status = registry.GetComponent<ecs::StatusComponent>(other.entity);
                status.hp_.SetBase(status.hp_.GetBase() - kWaveDamage);
            }
        }
    };

    registry.AddComponent<ecs::ColliderComponent>(wave, col);
    registry.AddComponent<CollisionResponseComponent>(wave, {});

    ParticleEffect* effect = ParticleManager::GetInstance()->Play("E_skill", trans.localPosition_);
    if (effect)
    {
        if (auto* e1 = effect->GetEmitter("e1"))
        {
            if (auto* scaleModule = e1->GetModule<InitialScaleModule>())
            {
                Vector3 s = { range, 1.0f, range };
                scaleModule->SetScaleRange(s, s);
            }
        }
    }
}

void PlayerActionSystem::FireHomingMissile(EntityID entity, Registry& registry)
{
    auto& skill = registry.GetComponent<SkillComponent>(entity);
    auto& trans = registry.GetComponent<TransformComponent>(entity);

    auto colSys = systemManager_->GetSystem<CollisionSystem>();
    if (!colSys)
    {
        return;
    }

    static constexpr float kHomingSearchRadius = 100.0f;
    static constexpr int kMaxMissiles = 5;

    std::vector<EntityID> markedTargets;
    std::vector<EntityID> ordinaryTargets;

    colSys->QueryNearbyEntities(trans.localPosition_, kHomingSearchRadius, [&](EntityID target)
    {
        if (!registry.HasComponent<ecs::TagComponent>(target))
        {
            return;
        }
        if (registry.GetComponent<ecs::TagComponent>(target).type != ecs::TagComponent::Type::Enemy)
        {
            return;
        }

        if (registry.HasComponent<ecs::InducedExplosionComponent>(target))
        {
            auto& stack = registry.GetComponent<ecs::InducedExplosionComponent>(target);
            if (stack.count_ > 0)
            {
                markedTargets.push_back(target);
                return;
            }
        }
        ordinaryTargets.push_back(target);
    });

    std::vector<EntityID> finalTargets;
    std::vector<EntityID>& candidates = markedTargets.empty() ? ordinaryTargets : markedTargets;

    if (!candidates.empty())
    {
        for (int i = 0; i < kMaxMissiles; ++i)
        {
            finalTargets.push_back(candidates[i % candidates.size()]);
        }
    }

    uint32_t missileCount = skill.eMissileCount_;

    Audio::GetInstance()->PlayWave("fire", false);

    for (uint32_t i = 0; i < missileCount; ++i)
    {
        EntityID proj = registry.CreateEntity();
        
        float offsetX = (i - (missileCount - 1) * 0.5f) * 0.5f;
        Vector3 spawnPos = { trans.localPosition_.x + offsetX, 0.5f, trans.localPosition_.z };
        registry.AddComponent<TransformComponent>(proj, { spawnPos, trans.localRotation_, {1.0f, 1.0f, 1.0f} });
        
        ProjectileComponent pc;
        pc.type_ = ProjectileComponent::Type::Lmb; 
        pc.speed_ = 40.0f;
        
        Matrix4x4 rot = MakeRotateMatrix(trans.localRotation_);
        pc.velocity_ = MathUtils::Transform({ 0, 0, 1 }, rot) * pc.speed_;

        pc.damage_ = 20.0f;
        pc.lifetime_ = 2.0f;
        
        EntityID target = kInvalidEntity;
        if (i < finalTargets.size())
        {
            target = finalTargets[i];
        }

        pc.isHoming_ = (target != kInvalidEntity);
        pc.targetEntity_ = target;
        
        pc.trailType_ = ProjectileComponent::TrailType::Homing;
        pc.trailId_ = HomingTrailManager::GetInstance().RegisterBulletManual();
        HomingTrailManager::GetInstance().UpdateBulletManual(pc.trailId_, spawnPos);
        
        registry.AddComponent<ProjectileComponent>(proj, pc);

        ecs::ColliderComponent col;
        col.type_ = ColliderType::Sphere;
        col.sphere_.radius = 0.5f;
        col.isTrigger_ = true;
        col.layer = CollisionLayer::kPlayerBullet;
        col.mask = CollisionLayer::kEnemy | CollisionLayer::kObstacle;

        col.onCollisionEnter = [this, &registry, proj](const ecs::CollisionPartnerInfo& other)
        {
            if (!registry.IsAlive(proj))
            {
                return;
            }
            if (registry.HasComponent<ecs::TagComponent>(other.entity) &&
                registry.GetComponent<ecs::TagComponent>(other.entity).type == ecs::TagComponent::Type::Enemy)
            {
                SpawnExplosion(other.entity, registry);
                registry.DestroyEntityDeferred(proj);
            }
        };

        registry.AddComponent<ecs::ColliderComponent>(proj, col);
        registry.AddComponent<CollisionResponseComponent>(proj, {});
    }
}

void PlayerActionSystem::SpawnDecoy(EntityID entity, Registry& registry)
{
    auto& skill = registry.GetComponent<SkillComponent>(entity);
    auto& trans = registry.GetComponent<TransformComponent>(entity);

    skill.activeDecoyEntities_.erase(
        std::remove_if(skill.activeDecoyEntities_.begin(), skill.activeDecoyEntities_.end(),
            [&registry](EntityID e) { return !registry.IsAlive(e); }),
        skill.activeDecoyEntities_.end());

    if (skill.activeDecoyEntities_.size() >= 1)
    {
        EntityID oldest = skill.activeDecoyEntities_[0];
        
        if (registry.HasComponent<DecoyComponent>(oldest))
        {
            auto& oldDc = registry.GetComponent<DecoyComponent>(oldest);
            if (oldDc.effect_)
            {
                oldDc.effect_->Stop();
                oldDc.effect_->SetAutoRemove(true);
                oldDc.effect_ = nullptr;
            }
        }

        registry.DestroyEntityDeferred(oldest);
        skill.activeDecoyEntities_.erase(skill.activeDecoyEntities_.begin());
    }

    EntityID decoy = registry.CreateEntity();
    Vector3 spawnPos = trans.localPosition_;
    spawnPos.y = 0.1f;
    registry.AddComponent<TransformComponent>(decoy, { spawnPos, {0,0,0}, {1,1,1} });

    DecoyComponent dc;
    dc.owner_ = entity;

    dc.effect_ = ParticleManager::GetInstance()->Play("Decoy_skill", spawnPos);
    if (dc.effect_)
    {
        dc.effect_->SetAutoRemove(false);
    }

    registry.AddComponent<DecoyComponent>(decoy, dc);
    
    ecs::TagComponent tag;
    tag.type = ecs::TagComponent::Type::Decoy;
    registry.AddComponent<ecs::TagComponent>(decoy, tag);

    registry.AddComponent<LifetimeComponent>(decoy, { 0.0f, skill.eDecoyDuration_ });

    InstancedRenderComponent render;
    render.modelName_ = "player";
    render.useInstancing_ = true;
    registry.AddComponent<InstancedRenderComponent>(decoy, render);

    skill.activeDecoyEntities_.push_back(decoy);

#ifdef _DEBUG
    LineManager::GetInstance()->DrawSphere(spawnPos, 5.0f, VectorColorCodes::Green);
#endif
}

void PlayerActionSystem::SpawnTurret(EntityID entity, Registry& registry)
{
    auto& skill = registry.GetComponent<SkillComponent>(entity);
    auto& trans = registry.GetComponent<TransformComponent>(entity);

    Vector3 spawnPos = trans.localPosition_;
    spawnPos.y = 1.0f;

    skill.activeTurretEntities_.erase(
        std::remove_if(skill.activeTurretEntities_.begin(), skill.activeTurretEntities_.end(),
            [&registry](EntityID e) { return !registry.IsAlive(e); }),
        skill.activeTurretEntities_.end());

    if (skill.activeTurretEntities_.size() >= skill.qMaxTurrets_)
    {
        EntityID oldest = skill.activeTurretEntities_[0];
        
        auto& tTrans = registry.GetComponent<TransformComponent>(oldest);
        tTrans.localPosition_ = spawnPos;
        tTrans.isDirty_ = true;
        
        skill.activeTurretEntities_.erase(skill.activeTurretEntities_.begin());
        skill.activeTurretEntities_.push_back(oldest);
        return;
    }

    EntityID turret = registry.CreateEntity();
    registry.AddComponent<TransformComponent>(turret, { spawnPos, {0,0,0}, {1.5f, 1.5f, 1.5f} });

    TurretComponent tc;
    tc.owner_ = entity;
    registry.AddComponent<TurretComponent>(turret, tc);

    InstancedRenderComponent render;
    render.modelName_ = "turret"; 
    render.useInstancing_ = true;
    registry.AddComponent<InstancedRenderComponent>(turret, render);

    skill.activeTurretEntities_.push_back(turret);

#ifdef _DEBUG
    LineManager::GetInstance()->DrawCube(spawnPos, 2.0f, VectorColorCodes::Blue);
#endif
}

void PlayerActionSystem::UpdateR(EntityID entity, Registry& registry, float)
{
    auto& skill = registry.GetComponent<SkillComponent>(entity);

    if (!skill.isBeamUnlocked_)
    {
        return;
    }

    if (!skill.isBeamReady_)
    {
        return;
    }

    if (Input::GetInstance()->TriggerKey(DIK_R))
    {
        Audio::GetInstance()->PlayWave("R", false);
        const float kBeamLength = 300.0f;
        const float kBeamWidth = 20.0f;
        const float kBeamDuration = 1.0f;
        const float kDamagePerSecond = 800.0f;

        EntityID beam = registry.CreateEntity();

        HierarchyComponent beamHier;
        beamHier.parent_ = entity;
        registry.AddComponent<HierarchyComponent>(beam, beamHier);

        if (!registry.HasComponent<HierarchyComponent>(entity))
        {
            registry.AddComponent<HierarchyComponent>(entity, {});
        }
        auto& playerHier = registry.GetComponent<HierarchyComponent>(entity);

        auto& currentBeamHier = registry.GetComponent<HierarchyComponent>(beam);
        currentBeamHier.nextSibling_ = playerHier.firstChild_;
        playerHier.firstChild_ = beam;

        TransformComponent beamTrans;
        beamTrans.localPosition_ = { 0.0f, 0.5f, kBeamLength * 0.5f };
        beamTrans.localScale_ = { 1.0f, 1.0f, 1.0f };
        registry.AddComponent<TransformComponent>(beam, beamTrans);

        ecs::ColliderComponent col;
        col.type_ = ColliderType::OBB;
        col.obb_.size = { kBeamWidth * 0.5f, 5.0f, kBeamLength * 0.5f };
        col.isTrigger_ = true;
        col.layer = CollisionLayer::kPlayerBullet;
        col.mask = CollisionLayer::kEnemy;

        auto collisionHandler = [this, &registry, kDamagePerSecond](const ecs::CollisionPartnerInfo& other)
        {
            if (!registry.IsAlive(other.entity))
            {
                return;
            }

            float dt = TimeManager::GetInstance().GetGameContext().deltaTime;

            if (registry.HasComponent<ecs::StatusComponent>(other.entity))
            {
                auto& status = registry.GetComponent<ecs::StatusComponent>(other.entity);
                status.hp_.SetBase(status.hp_.GetBase() - (kDamagePerSecond * dt));
            }

            if (registry.HasComponent<ecs::InducedExplosionComponent>(other.entity))
            {
                auto& stack = registry.GetComponent<ecs::InducedExplosionComponent>(other.entity);
                stack.count_++;
                if (stack.count_ >= ecs::InducedExplosionComponent::kMaxCount)
                {
                    SpawnExplosion(other.entity, registry);
                }
            }
            else
            {
                registry.AddComponent<ecs::InducedExplosionComponent>(other.entity, { 1 });
            }
        };

        col.onCollisionEnter = collisionHandler;
        col.onCollisionStay = collisionHandler;

        registry.AddComponent<ecs::ColliderComponent>(beam, col);
        registry.AddComponent<CollisionResponseComponent>(beam, {});

        registry.AddComponent<LifetimeComponent>(beam, { 0.0f, kBeamDuration });

        skill.beamCharge_ = 0.0f;
        skill.isBeamReady_ = false;

        if (ParticleManager* pm = ParticleManager::GetInstance())
        {
            if (registry.HasComponent<TransformComponent>(entity))
            {
                auto& playerTrans = registry.GetComponent<TransformComponent>(entity);

                ParticleEffect* effect = pm->Play("R_skill", playerTrans.localPosition_);
                if (effect)
                {
                    float yaw = playerTrans.localRotation_.y;
                    Vector3 forward = { std::sin(yaw), 0.0f, std::cos(yaw) };

                    if (auto* e2 = effect->GetEmitter("e2"))
                    {
                        e2->SetFollowOffset(forward * 60.0f);
                    }
                    if (auto* e3 = effect->GetEmitter("e3"))
                    {
                        e3->SetFollowOffset(forward * 50.0f);
                    }

                    float radToDeg = 180.0f / 3.14159265f;
                    Vector3 rotDeg = {
                        -90.0f,
                        playerTrans.localRotation_.y * radToDeg,
                        0.0f
                    };

                    for (size_t i = 0; i < effect->GetEmitterCount(); ++i)
                    {
                        if (auto* emitter = effect->GetEmitter(i))
                        {
                            if (auto* rotModule = emitter->GetModule<InitialRotationModule>())
                            {
                                rotModule->SetRotationRange(rotDeg, rotDeg);
                            }
                        }
                    }

                    skill.activeBeamParticle_ = effect;
                    skill.beamActiveTimer_ = kBeamDuration;
                }
            }
        }
    }
}

void PlayerActionSystem::SpawnExplosion(EntityID sourceEntity, Registry& registry)
{
    if (!registry.IsAlive(sourceEntity))
    {
        return;
    }
    if (!registry.HasComponent<TransformComponent>(sourceEntity))
    {
        return;
    }

    Vector3 expPos = registry.GetComponent<TransformComponent>(sourceEntity).localPosition_;

    EntityID explosion = registry.CreateEntity();
    registry.AddComponent<TransformComponent>(explosion, { expPos, {0,0,0}, {1,1,1} });
    registry.AddComponent<LifetimeComponent>(explosion, { 0.0f, 0.1f });

    ecs::ColliderComponent col;
    col.type_ = ColliderType::Sphere;
    col.sphere_.radius = ecs::InducedExplosionComponent::kExplosionRadius;
    col.isTrigger_ = true;
    col.layer = CollisionLayer::kPlayerBullet;
    col.mask = CollisionLayer::kEnemy;

    col.onCollisionEnter = [&registry, expPos](const ecs::CollisionPartnerInfo& other)
    {
        if (registry.HasComponent<ecs::TagComponent>(other.entity) &&
            registry.GetComponent<ecs::TagComponent>(other.entity).type == ecs::TagComponent::Type::Enemy)
        {
            ParticleManager::GetInstance()->Play("E_explosion", expPos);
            Audio::GetInstance()->PlayWave("explosion", false);
            if (registry.HasComponent<ecs::StatusComponent>(other.entity))
            {
                auto& status = registry.GetComponent<ecs::StatusComponent>(other.entity);
                status.hp_.SetBase(status.hp_.GetBase() - ecs::InducedExplosionComponent::kExplosionDamage);
                if (status.hp_.GetBase() <= 0.0f)
                {
                    status.isAlive_ = false;
                }
            }
        }
    };

    registry.AddComponent<ecs::ColliderComponent>(explosion, col);
    registry.AddComponent<CollisionResponseComponent>(explosion, {});

    if (registry.HasComponent<ecs::StatusComponent>(sourceEntity))
    {
        auto& status = registry.GetComponent<ecs::StatusComponent>(sourceEntity);
        status.hp_.SetBase(status.hp_.GetBase() - ecs::InducedExplosionComponent::kExplosionDamage);
        if (status.hp_.GetBase() <= 0.0f)
        {
            status.isAlive_ = false;
        }
        ParticleManager::GetInstance()->Play("E_explosion", expPos);
        Audio::GetInstance()->PlayWave("explosion", false);
    }

    registry.RemoveComponent<ecs::InducedExplosionComponent>(sourceEntity);

#ifdef _DEBUG
    LineManager::GetInstance()->DrawSphere(expPos, ecs::InducedExplosionComponent::kExplosionRadius, VectorColorCodes::Red);
#endif
}

