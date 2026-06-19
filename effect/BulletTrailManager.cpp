#include "BulletTrailManager.h"
#include "effects/particle/ParticleManager.h"
#include "effects/particle/ParticleEffect.h"
#include "effects/particle/ParticleEmitter.h"
#include "effects/particle/module/update/RibbonModules.h"

BulletTrailManager& BulletTrailManager::GetInstance()
{
    static BulletTrailManager instance;
    return instance;
}

void BulletTrailManager::Initialize()
{
    if (initialized_) return;

    // JSONからエフェクト定義を読み込み
    ParticleManager::GetInstance()->LoadEffectDefinition(kEffectName, kEffectJsonPath);

    // 単一のエフェクトを作成（自動削除無効）
    effect_ = ParticleManager::GetInstance()->Play(kEffectName, Vector3(0, 0, 0));
    if (effect_)
    {
        effect_->SetAutoRemove(false);  // 自動削除を無効化

        // 最初のエミッターからMultiSourceRibbonModuleを取得
        if (effect_->GetEmitterCount() > 0)
        {
            ParticleEmitter* emitter = effect_->GetEmitter(static_cast<size_t>(0));
            if (emitter)
            {
                // モジュールを探索してMultiSourceRibbonModuleを取得
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

uint32_t BulletTrailManager::RegisterBullet(Transform* bulletTransform)
{
    if (!initialized_) Initialize();

    if (!multiSourceModule_)
    {
        // MultiSourceRibbonModuleがない場合は0を返す（エラー）
        return 0;
    }

    // MultiSourceRibbonModuleにソースを登録
    return multiSourceModule_->RegisterSource(bulletTransform);
}

uint32_t BulletTrailManager::RegisterBulletManual()
{
    if (!initialized_) Initialize();
    if (!multiSourceModule_) return 0;

    return multiSourceModule_->RegisterSourceManual();
}

void BulletTrailManager::UpdateBulletManual(uint32_t trailId, const Vector3& position)
{
    if (multiSourceModule_ && trailId > 0)
    {
        multiSourceModule_->UpdateSourcePosition(trailId, position);
    }
}

void BulletTrailManager::UnregisterBullet(uint32_t trailId)
{
    if (multiSourceModule_ && trailId > 0)
    {
        // MultiSourceRibbonModuleからソースを解除
        multiSourceModule_->UnregisterSource(trailId);
    }
}

void BulletTrailManager::Clear()
{
    if (multiSourceModule_)
    {
        multiSourceModule_->ClearSources();
    }

    // エミッター内のパーティクルもクリア
    if (effect_ && effect_->GetEmitterCount() > 0)
    {
        ParticleEmitter* emitter = effect_->GetEmitter(static_cast<size_t>(0));
        if (emitter)
        {
            emitter->ClearParticles();
        }
    }

    // シーン終了時にポインタをクリア
    // ParticleManagerがエフェクトを削除する可能性があるため、
    // 次回Initialize()で再取得する必要がある
    effect_ = nullptr;
    multiSourceModule_ = nullptr;
    initialized_ = false;
}
