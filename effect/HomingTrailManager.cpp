#include "HomingTrailManager.h"
#include "effects/particle/ParticleManager.h"
#include "effects/particle/ParticleEffect.h"
#include "effects/particle/ParticleEmitter.h"
#include "effects/particle/module/update/RibbonModules.h"

HomingTrailManager& HomingTrailManager::GetInstance()
{
    static HomingTrailManager instance;
    return instance;
}

void HomingTrailManager::Initialize()
{
    if (initialized_) return;

    // JSONからエフェクト定義を読み込み
    ParticleManager::GetInstance()->LoadEffectDefinition(kEffectName, kEffectJsonPath);

    // 単一のエフェクトを作成（自動削除無効）
    effect_ = ParticleManager::GetInstance()->Play(kEffectName, Vector3(0, 0, 0));
    if (effect_)
    {
        effect_->SetAutoRemove(false);

        if (effect_->GetEmitterCount() > 0)
        {
            ParticleEmitter* emitter = effect_->GetEmitter(static_cast<size_t>(0));
            if (emitter)
            {
                for (size_t i = 0; i < emitter->GetModuleCount(); ++i)
                {
                    IModule* module = emitter->GetModule(i);
                    if (auto* msrModule = dynamic_cast<MultiSourceRibbonModule*>(module))
                    {
                        multiSourceModule_ = msrModule;
                        break;
                    }
                }
            }
        }
    }

    initialized_ = true;
}

uint32_t HomingTrailManager::RegisterBulletManual()
{
    if (!initialized_) Initialize();
    if (!multiSourceModule_) return 0;

    return multiSourceModule_->RegisterSourceManual();
}

void HomingTrailManager::UpdateBulletManual(uint32_t trailId, const Vector3& position)
{
    if (multiSourceModule_ && trailId > 0)
    {
        multiSourceModule_->UpdateSourcePosition(trailId, position);
    }
}

void HomingTrailManager::UnregisterBullet(uint32_t trailId)
{
    if (multiSourceModule_ && trailId > 0)
    {
        multiSourceModule_->UnregisterSource(trailId);
    }
}

void HomingTrailManager::Clear()
{
    if (multiSourceModule_)
    {
        multiSourceModule_->ClearSources();
    }

    if (effect_ && effect_->GetEmitterCount() > 0)
    {
        ParticleEmitter* emitter = effect_->GetEmitter(static_cast<size_t>(0));
        if (emitter)
        {
            emitter->ClearParticles();
        }
    }

    effect_ = nullptr;
    multiSourceModule_ = nullptr;
    initialized_ = false;
}
