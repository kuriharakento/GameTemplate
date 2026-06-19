#include "EnemySpawnSystem.h"
#include "engine/ecs/Registry.h"
#include "engine/ecs/components/TransformComponent.h"
#include "engine/ecs/components/TagComponent.h"
#include "engine/ecs/components/EnemyAIComponent.h"
#include "application/ecs/components/StatusComponent.h"
#include "application/ecs/components/ImpactChargeComponent.h"
#include "engine/ecs/components/InstancedRenderComponent.h"
#include "engine/ecs/components/ColliderComponent.h"
#include "engine/ecs/components/CollisionResponseComponent.h"
#include "application/ecs/components/EnemyTypeComponent.h"
#include "application/ecs/components/EnemyChargerComponent.h"
#include "application/ecs/components/PlayerProgressionComponent.h"
#include "application/ecs/CollisionConfig.h"
#include "engine/time/TimeManager.h"
#include "math/MathUtils.h"
#include "math/VectorColorCodes.h"
#include "engine/manager/graphics/LineManager.h"
#include "engine/manager/scene/CameraManager.h"
#include "engine/graphics/3d/Object3dCommon.h"
#include "engine/manager/scene/LightManager.h"
#include <random>

using namespace ecs;


void EnemySpawnSystem::Initialize(Object3dCommon* object3dCommon, LightManager* lightManager, CameraManager* cameraManager)
{
    object3dCommon_ = object3dCommon;
    lightManager_ = lightManager;
    cameraManager_ = cameraManager;
}

void EnemySpawnSystem::Update(Registry& registry)
{
    float dt = TimeManager::GetInstance().GetGameContext().deltaTime;
    if (dt <= 0.0f)
    {
        dt = 0.0166f;
    }

    elapsedTime_ += dt;

    // プレイヤーレベル取得
    uint32_t playerLevel = 1;
    auto progView = registry.View<PlayerProgressionComponent>();
    if (progView && progView->GetSize() > 0)
    {
        playerLevel = progView->GetDataFromDenseIndex(0).level_;
    }

    // 敵の生存数カウント
    uint32_t currentEnemyCount = 0;
    auto tagView = registry.View<ecs::TagComponent>();
    if (tagView)
    {
        for (uint32_t i = 0; i < tagView->GetSize(); ++i)
        {
            if (tagView->GetDataFromDenseIndex(i).type == ecs::TagComponent::Type::Enemy)
            {
                currentEnemyCount++;
            }
        }
    }

    uint32_t currentMaxEnemies = GetMaxEnemies(playerLevel);

    // 通常スポーン
    tickTimer_ += dt;
    if (tickTimer_ >= 1.0f)
    {
        tickTimer_ = 0.0f;
        
        uint32_t tickAmount = (playerLevel >= 5) ? 4 : playerLevel;
        for (uint32_t i = 0; i < tickAmount; ++i)
        {
            if (currentEnemyCount >= currentMaxEnemies)
            {
                break;
            }
            SpawnEnemy(registry);
            currentEnemyCount++;
        }
    }

    // バースト増援
    if (playerLevel >= 4)
    {
        burstTimer_ += dt;
        if (burstTimer_ >= 10.0f)
        {
            burstTimer_ = 0.0f;

            uint32_t burstSize = 15;
            for (uint32_t i = 0; i < burstSize; ++i)
            {
                if (currentEnemyCount >= currentMaxEnemies)
                {
                    break;
                }
                SpawnEnemy(registry);
                currentEnemyCount++;
            }
        }
    }
}

uint32_t EnemySpawnSystem::GetMaxEnemies(uint32_t level)
{
    if (level >= 5) return kMaxEnemiesLv5;
    if (level == 4) return kMaxEnemiesLv4;
    if (level == 3) return kMaxEnemiesLv3;
    if (level == 2) return kMaxEnemiesLv2;
    return kMaxEnemiesLv1;
}

void EnemySpawnSystem::SpawnEnemy(Registry& registry)
{
    // プレイヤー位置取得
    Vector3 playerPos = { 0, 0, 0 };
    auto tagView = registry.View<ecs::TagComponent>();
    if (tagView)
    {
        for (uint32_t i = 0; i < tagView->GetSize(); ++i)
        {
            if (tagView->GetDataFromDenseIndex(i).type == ecs::TagComponent::Type::Player)
            {
                EntityID player = tagView->GetEntityFromDenseIndex(i);
                if (registry.HasComponent<TransformComponent>(player))
                {
                    playerPos = registry.GetComponent<TransformComponent>(player).localPosition_;
                }
                break;
            }
        }
    }

    // 円環状のランダム位置
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDist(innerRadius_, outerRadius_);

    float angle = angleDist(gen);
    float radius = radiusDist(gen);
    Vector3 spawnOffset = { std::cos(angle) * radius, 0, std::sin(angle) * radius };
    Vector3 spawnPos = playerPos + spawnOffset;

    Vector3 meleeScale = { 2.5f, 2.5f, 2.5f };
    Vector3 chargerScale = { 4.5f, 4.5f, 4.5f };
    spawnPos.y = meleeScale.y * 0.5f;

    EntityID enemy = registry.CreateEntity();
    
    ecs::TagComponent tag;
    tag.type = ecs::TagComponent::Type::Enemy;
    registry.AddComponent<ecs::TagComponent>(enemy, tag);
    registry.AddComponent<TransformComponent>(enemy, { spawnPos, {0,0,0}, meleeScale });
    
    ecs::StatusComponent status;
    status.hp_.SetBase(100.0f);
    status.moveSpeed_.SetBase(3.5f + static_cast<float>(rand() % 100) / 100.0f * 1.0f);
    registry.AddComponent<ecs::StatusComponent>(enemy, std::move(status));

    EnemyAIComponent ai;
    ai.targetEntity_ = kInvalidEntity; 
    registry.AddComponent<EnemyAIComponent>(enemy, std::move(ai));

    registry.AddComponent<ImpactChargeComponent>(enemy, {});

    ecs::ColliderComponent col;
    col.type_ = ColliderType::Sphere;
    col.sphere_.radius = 0.5f;
    col.previousPosition_ = spawnPos;
    col.layer = CollisionLayer::kEnemy;
    col.mask = CollisionLayer::kPlayer | CollisionLayer::kPlayerBullet;

    col.onCollisionEnter = [&registry, enemy](const ecs::CollisionPartnerInfo& other)
    {
        if (registry.HasComponent<ecs::ColliderComponent>(other.entity))
        {
            auto& otherCol = registry.GetComponent<ecs::ColliderComponent>(other.entity);
            
            if (otherCol.layer & CollisionLayer::kPlayerBullet)
            {
                // PlayerActionSystem 側で処理
            }
            else if (otherCol.layer & CollisionLayer::kPlayer)
            {
                if (registry.HasComponent<ecs::StatusComponent>(other.entity))
                {
                    auto& status = registry.GetComponent<ecs::StatusComponent>(other.entity);
                    status.hp_.SetBase(status.hp_.GetBase() - 1.0f);
                }
                registry.DestroyEntityDeferred(enemy);
            }
        }
    };
    registry.AddComponent<ecs::ColliderComponent>(enemy, col);
    registry.AddComponent<CollisionResponseComponent>(enemy, {});

    EnemyTypeComponent typeComp;
    InstancedRenderComponent render;

    if ((rand() % 100) < 30)
    {
        typeComp.type = EnemyType::Charger;
        EnemyChargerComponent chargerComp;
        registry.AddComponent<EnemyChargerComponent>(enemy, chargerComp);

        if (registry.HasComponent<TransformComponent>(enemy))
        {
            auto& trans = registry.GetComponent<TransformComponent>(enemy);
            trans.localScale_ = chargerScale;
            trans.localPosition_.y = chargerScale.y * 0.5f;
            trans.isDirty_ = true;
        }

        if (registry.HasComponent<ecs::ColliderComponent>(enemy))
        {
            auto& c = registry.GetComponent<ecs::ColliderComponent>(enemy);
            c.sphere_.radius = 0.6f;
        }

        render.modelName_ = "tank_enemy";
    }
    else
    {
        typeComp.type = EnemyType::Melee;
        render.modelName_ = "weak_enemy";
    }
    registry.AddComponent<EnemyTypeComponent>(enemy, typeComp);
    registry.AddComponent<InstancedRenderComponent>(enemy, render);
}

void EnemySpawnSystem::Draw(Registry& registry, Camera* camera, LightManager* lightManager, ShadowMapManager* shadowMapManager)
{
    (void)camera; (void)lightManager; (void)shadowMapManager;
#ifdef _DEBUG
    Vector3 playerPos = { 0, 0, 0 };
    auto tagView = registry.View<ecs::TagComponent>();
    if (tagView)
    {
        for (uint32_t i = 0; i < tagView->GetSize(); ++i)
        {
            if (tagView->GetDataFromDenseIndex(i).type == ecs::TagComponent::Type::Player)
            {
                EntityID player = tagView->GetEntityFromDenseIndex(i);
                if (registry.HasComponent<TransformComponent>(player))
                {
                    playerPos = registry.GetComponent<TransformComponent>(player).localPosition_;
                }
                break;
            }
        }
    }

    auto* lm = LineManager::GetInstance();
    if (!lm)
    {
        return;
    }

    float h = 0.05f;
    const uint32_t kSegments = 32;
    
    // 内径
    for (uint32_t i = 0; i < kSegments; ++i)
    {
        float a1 = (float)i / kSegments * 2.0f * 3.14159f;
        float a2 = (float)(i + 1) / kSegments * 2.0f * 3.14159f;
        Vector3 p1 = playerPos + Vector3(cos(a1) * innerRadius_, h, sin(a1) * innerRadius_);
        Vector3 p2 = playerPos + Vector3(cos(a2) * innerRadius_, h, sin(a2) * innerRadius_);
        lm->DrawLine(p1, p2, VectorColorCodes::Cyan);
    }

    // 外径
    for (uint32_t i = 0; i < kSegments; ++i)
    {
        float a1 = (float)i / kSegments * 2.0f * 3.14159f;
        float a2 = (float)(i + 1) / kSegments * 2.0f * 3.14159f;
        Vector3 p1 = playerPos + Vector3(cos(a1) * outerRadius_, h, sin(a1) * outerRadius_);
        Vector3 p2 = playerPos + Vector3(cos(a2) * outerRadius_, h, sin(a2) * outerRadius_);
        lm->DrawLine(p1, p2, VectorColorCodes::Lime);
    }
#endif
}

