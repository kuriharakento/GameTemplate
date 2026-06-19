#include "MoveComponent.h"
#include "engine/gameobject/base/GameObject.h"
#include "math/MathUtils.h"
#include <cmath>

#include "AssaultRifleComponent.h"
#include "KnifeEnemyBehavior.h"
#include "base/Logger.h"
#include "math/Easing.h"
#include "time/TimeManager.h"
#include "application/GameObject/Combatable/character/enemy/EnemyManager.h"
#include "application/GameObject/Combatable/character/player/Player.h"
#include "application/GameObject/component/action/StatusComponent.h"
#include "time/Timer.h"
#include "time/TimerManager.h"
#include "base/Camera.h"
#include "base/WinApp.h"
#include "engine/manager/effect/PostProcessManager.h"
#include "effects/particle/ParticleEffect.h"

namespace GameObjectComponent
{
    // コンストラクタ：マネージャーの初期化
    MoveComponent::MoveComponent(EnemyManager* enemyManager, CameraManager* camera, PostProcessManager* postProcessManager)
    {
        // 敵マネージャーのポインタを保存
        enemyManager_ = enemyManager;
        // カメラのポインタを保存
        camera_ = camera->GetActiveCamera();
        // ポストプロセスマネージャーのポインタを保存
        postProcessManager_ = postProcessManager;
        // 軌跡パーティクルを読み込み
        trailEffect_ = ParticleManager::GetInstance()->Load("player_trail", "./Resources/json/particle/player_trail.json");
    }   

    // フレームごとの更新処理
    void MoveComponent::Update(::GameObject* owner)
    {
        // 軌跡パーティクルの位置を更新
        trailEffect_->SetPosition(owner->GetPosition());

        // StatusComponentから移動速度を取得
        auto status = owner->GetComponent<StatusComponent>();
        if (status)
        {
            moveSpeed_ = status->moveSpeed.GetValue();
        }
        else
        {
            // 取得できない場合は処理を中断
            moveSpeed_ = 0.0f;
            Logger::Log("MoveComponent::Update: StatusComponent not found!\n");
            return;
        }

        // デルタタイムを取得（プレイヤーはリアルタイム、それ以外はゲーム時間）
        float deltaTime = 0.0f;
        auto ownerAsPlayer = dynamic_cast<::Player*>(owner);

        if (ownerAsPlayer)
        {
            deltaTime = TimeManager::GetInstance().GetGameContext().realDeltaTime;
        }
        else
        {
            deltaTime = TimeManager::GetInstance().GetGameContext().deltaTime;
        }

        // クールダウンタイマー更新
        if (dodgeCooldownTimer_ > 0.0f)
        {
            dodgeCooldownTimer_ -= deltaTime;
        }

        // 回避タイマー更新
        if (dodgeTimer_ > 0.0f)
        {
            // 前回の位置を保存
            Vector3 prevPosition = owner->GetPosition();

            // 回避タイマーを更新
            float previousTime = dodgeTimer_ / dodgeDuration_;
            dodgeTimer_ -= deltaTime;
            float currentTime = (std::max)(0.0f, dodgeTimer_) / dodgeDuration_;

            // 回避処理
            if (isDodging_)
            {
                // イージングを使用して滑らかな回避動作
                float progress = 1.0f - currentTime;
                Vector3 newPosition = MathUtils::Lerp(dodgeStartPosition_, dodgeTargetPosition_, progress);
                owner->SetPosition(newPosition);

                // 回避中も向きを滑らかに補間
                UpdateRotation(owner, dodgeDirection_);

                // 回避エフェクト表示
                effectTimer_ -= deltaTime;
                if (effectTimer_ <= 0.0f)
                {
                    PlayDodgeEffect(owner);
                    effectTimer_ = effectInterval_;
                }
            }

            // 回避終了時の処理
            if (dodgeTimer_ <= 0.0f)
            {
                // 状態をリセット
                dodgeTimer_ = 0.0f;
                isDodging_ = false;
                wasEffectPlayed_ = false;
                dodgeCooldownTimer_ = dodgeCooldown_;
                if (!isInBulletTime_)
                {
                    //　バレットタイム中でなければそのまま停止させる
                    trailEffect_->Stop();
                }
            }
        }

        // 無敵タイマー更新
        if (invincibleTimer_ > 0.0f)
        {
            invincibleTimer_ -= deltaTime;
            if (invincibleTimer_ < 0.0f)
            {
                invincibleTimer_ = 0.0f;
            }
        }

        // 入力処理（回避中は処理しない）
        if (!isDodging_)
        {
            // 回避を最優先でチェック
            ProcessDodge(owner);
            if (!isDodging_)
            {
                // 回避が始まらなかったら通常移動
                ProcessMovement(owner, deltaTime);
            }

            // プレイヤーの場合は常にマウスの方向を向く処理を行う
            if (auto player = dynamic_cast<::Player*>(owner))
            {
                ProcessLookAtMouse(owner);
            }
        }

        // バレットタイム処理
        ProcessBulletTime(owner);

        // バレットタイム中のエフェクト強度のイージング（線形進行度の更新）
        if (isInBulletTime_) {
            effectTransitionProgress_ += deltaTime / effectTransitionDuration_;
            if (effectTransitionProgress_ > 1.0f) effectTransitionProgress_ = 1.0f;
        } else {
            effectTransitionProgress_ -= deltaTime / effectTransitionDuration_;
            if (effectTransitionProgress_ < 0.0f) effectTransitionProgress_ = 0.0f;
        }

        // 選択されたイージング関数で進行度を曲線に変換して強度を決定
        float t = effectTransitionProgress_;
        switch (effectEasingType_) {
            case EffectEasingType::EaseOutQuad:   effectIntensity_ = EaseOutQuad(t); break;
            case EffectEasingType::EaseInOutSine: effectIntensity_ = EaseInOutSine(t); break;
            case EffectEasingType::EaseOutExpo:   effectIntensity_ = EaseOutExpo(t); break;
            case EffectEasingType::EffectEasingType_EaseOutBack:   effectIntensity_ = EaseOutBack(t); break;
            default:                              effectIntensity_ = t; break; // 線形
        }

        // エフェクトの適用
        if (postProcessManager_) {
            // 強度がわずかでもあればエフェクトを有効化
            bool isEffectActive = effectIntensity_ > 0.01f;

            if (postProcessManager_->grayscaleEffect_) {
                postProcessManager_->grayscaleEffect_->SetEnabled(isEffectActive);
                // グレースケールは1.0が最大強度
                postProcessManager_->grayscaleEffect_->SetIntensity(effectIntensity_);
            }
            if (postProcessManager_->vignetteEffect_) {
                postProcessManager_->vignetteEffect_->SetEnabled(isEffectActive);
                // ビネットは 1.0 だと強すぎるため、0.8 程度に抑える
                postProcessManager_->vignetteEffect_->SetIntensity(effectIntensity_ * 0.8f);
            }
            if (postProcessManager_->noiseEffect_) {
                //postProcessManager_->noiseEffect_->SetEnabled(isEffectActive);
                // ノイズは 0.2 程度が適正
                postProcessManager_->noiseEffect_->SetIntensity(effectIntensity_ * 0.2f);
                
                // ノイズをアニメーションさせるためにTimeを加算
                if (isEffectActive) {
                    float currentTime = postProcessManager_->noiseEffect_->GetTime();
                    postProcessManager_->noiseEffect_->SetTime(currentTime + deltaTime);
                }
            }
        }

        // アニメーション制御（スキニングモデルを使っている場合）
        if (auto* skinned = owner->GetSkinnedObject3d())
        {
            // 回避中は制御しない（あるいは回避アニメーションなど）
            if (!isDodging_)
            {
                if (hasMovementInput_)
                {
                    skinned->PlayAnimation(0, true);
                }
                else
                {
                    skinned->StopAnimation();
                }
            }
        }
    }

    // 向きを滑らかに補間する処理
    void MoveComponent::UpdateRotation(::GameObject* owner, const Vector3& direction)
    {
        // 移動方向がある場合のみ向きを更新
        if (direction.Length() > kMovementInputThreshold)
        {
            // 正規化された方向ベクトル
            Vector3 normalizedDir = direction;
            normalizedDir.NormalizeSelf();

            // Y軸回りの目標回転角度を計算
            float targetRotationY = atan2f(normalizedDir.x, normalizedDir.z);

            // 現在の回転を取得
            Vector3 currentRotation = owner->GetRotation();

            // Y軸の回転のみ、最短経路で補間
            float easedRotationY = MathUtils::LerpAngle(
                currentRotation.y,
                targetRotationY,
                kDodgeRotationInterpolation
            );

            // 回転を更新
            owner->SetRotation({ currentRotation.x, easedRotationY, currentRotation.z });
        }
    }

    // バレットタイム処理
    void MoveComponent::ProcessBulletTime(::GameObject* owner)
    {
        // バレットタイム中、クールダウン中、または回避中でない場合はスキップ
        if (isInBulletTime_ || IsBulletTimeCoolingDown() || !isDodging_) { return; }

        // 敵の攻撃をチェック (ECS移行により、弾やナイフの判定は別途System化または再実装が必要)
        auto registry = enemyManager_->GetRegistry();
        if (!registry) return;

        // TODO: 今後、ECS管理された敵の `EnemyBulletComponent` や `EnemyAttackStateComponent` から
        // バレットタイムのトリガー判定を行うよう改修する。
        // 以下は旧GameObjectベースのコードからの移行措置として一旦コメントアウト

        /*
        const auto* transformArr = registry->GetComponentArray<TransformComponent>();
        // ... 弾とプレイヤーの距離判定等
        */

    }

    // バレットタイム発動
    void MoveComponent::ActivateBulletTime(::GameObject* owner)
    {
        isInBulletTime_ = true;

        // CarnageModeと同じようにPlayer*をキャッシュしてバフを付与する
        player_ = dynamic_cast<::Player*>(owner);
        ApplyBulletTimeBuffs();

        //  エフェクトの再生
        ParticleManager::GetInstance()->Play("dodge_effect", owner->GetPosition());

        // バレットタイムタイマーを作成
        auto bulletTime = std::make_unique<Timer>("bulletTime", bulletTimeDuration_, DeltaTimeType::RealDeltaTime);
        bulletTime->SetOnStart([this]() {
            // ゲーム時間をスローモーションに
            TimeManager::GetInstance().SetGameTimeScale(bulletTimeScale_);

            // 軌跡エフェクトの再生
            trailEffect_->Play();
        });
        bulletTime->SetOnFinish([this, owner]() {
            // ゲーム時間を通常に戻す
            TimeManager::GetInstance().SetGameTimeScale(kNormalTimeScale);

            // バレットタイム終了
            isInBulletTime_ = false;

            // バレットタイム終了のエフェクトを再生
            ParticleManager::GetInstance()->Play("bulletTime_finish_effect", owner->GetPosition());

            // バフを解除
            RemoveBulletTimeBuffs();

            // 軌跡エフェクトの終了
            trailEffect_->Stop();

            // クールダウンタイマーを作成
            auto timer = std::make_unique<Timer>("bulletTimeCooldown", bulletTimeCooldown_, DeltaTimeType::RealDeltaTime);
            timer->SetOnFinish([this]() {
                // クールダウン終了
            });
            TimerManager::GetInstance().AddTimer(std::move(timer));
        });
        TimerManager::GetInstance().AddTimer(std::move(bulletTime));
    }

    // バレットタイム中のバフを付与
    void MoveComponent::ApplyBulletTimeBuffs()
    {
        if (!player_) return;
        auto status = player_->GetComponent<StatusComponent>();
        if (!status) return;

        // 移動速度バフ
        status->moveSpeed.AddBuff(BuffConfig("BulletTimeMoveSpeed", moveSpeedBuff_, BuffType::Percentage));
        // 射撃レートバフ
        status->fireRateMultiplier.AddBuff(BuffConfig("BulletTimeFireRate", fireRateBuff_, BuffType::Percentage));
    }

    // バレットタイム中のバフを解除
    void MoveComponent::RemoveBulletTimeBuffs()
    {
        if (!player_) return;
        auto status = player_->GetComponent<StatusComponent>();
        if (!status) return;

        status->moveSpeed.RemoveBuff("BulletTimeMoveSpeed");
        status->fireRateMultiplier.RemoveBuff("BulletTimeFireRate");
        player_ = nullptr;
    }

    // 回避動作の進行度を取得
    float MoveComponent::GetDodgeProgress() const
    {
        if (!IsDodging()) return 0.0f;
        return 1.0f - (dodgeTimer_ / dodgeDuration_);
    }

    // バレットタイムのクールダウン中かどうかを取得
    bool MoveComponent::IsBulletTimeCoolingDown() const
    {
        Timer* cooldownTimer = TimerManager::GetInstance().GetTimer("bulletTimeCooldown");
        if (cooldownTimer && cooldownTimer->IsRunning())
        {
            return true;
        }
        return false;
    }

    // バレットタイムのクールダウン進行度を取得
    float MoveComponent::GetBulletTimeCooldownProgress() const
    {
        Timer* cooldownTimer = TimerManager::GetInstance().GetTimer("bulletTimeCooldown");
        if (cooldownTimer && cooldownTimer->IsRunning())
        {
            // リロードUIと同じように、時間経過とともに 0.0 から 1.0 に増えるようにする
            return 1.0f - (cooldownTimer->GetRemainingTime() / cooldownTimer->GetDuration());
        }
        return 0.0f;
    }

    // 移動処理
    void MoveComponent::ProcessMovement(::GameObject* owner, float deltaTime)
    {
        // 回避中は通常移動しない
        if (IsDodging()) return;

        // 移動方向を取得
        Vector3 moveDirection = GetMovementDirection();
        hasMovementInput_ = moveDirection.Length() > kMovementInputThreshold;

        // 移動処理
        if (hasMovementInput_)
        {
            moveDirection.NormalizeSelf();
            owner->SetPosition(owner->GetPosition() + moveDirection * moveSpeed_ * deltaTime);

            // プレイヤー以外（敵など）の場合のみ、移動方向に向きを変える
            // プレイヤーは ProcessLookAtMouse で常にマウス方向を向くため
            if (!dynamic_cast<::Player*>(owner))
            {
                UpdateRotation(owner, moveDirection);
            }
        }
    }

    // 回避処理
    void MoveComponent::ProcessDodge(::GameObject* owner)
    {
        // すでに回避中なら処理しない
        if (isDodging_) return;

        // スペースキーで回避（クールダウン終了時のみ）
        if (Input::GetInstance()->TriggerKey(DIK_SPACE) && dodgeCooldownTimer_ <= 0.0f)
        {
            // 回避方向の決定（移動方向優先、なければ向いている方向）
            Vector3 moveDirection = GetMovementDirection();

            if (moveDirection.Length() > kMovementInputThreshold)
            {
                // 移動入力がある場合はその方向に回避
                dodgeDirection_ = moveDirection.Normalize();
            }
            else
            {
                // 移動入力がない場合は前方向に回避
                Vector3 rotation = owner->GetRotation();
                float angleY = rotation.y;
                dodgeDirection_ = Vector3(sin(angleY), 0, cos(angleY));
            }

            // 回避開始位置と目標位置を計算
            dodgeStartPosition_ = owner->GetPosition();
            dodgeTargetPosition_ = dodgeStartPosition_ + dodgeDirection_ * dodgeDistance_;

            // 回避開始
            isDodging_ = true;
            isFirstDodgeFrame_ = true;
            wasEffectPlayed_ = false;
            dodgeTimer_ = dodgeDuration_;
            invincibleTimer_ = dodgeInvincibleTime_;
            effectTimer_ = 0.0f;

            // 軌跡エフェクトを再生
            trailEffect_->Play();
        }
        else
        {
            isFirstDodgeFrame_ = false;
        }
    }

    // 回避エフェクトを再生
    void MoveComponent::PlayDodgeEffect(::GameObject* owner)
    {
        // NOTE:後でパーティクルを出す
    }

    // 移動方向を取得
    Vector3 MoveComponent::GetMovementDirection() const
    {
        Vector3 inputDirection(0, 0, 0);

        // WASDキーの入力を取得
        if (Input::GetInstance()->PushKey(DIK_W)) inputDirection.z += 1.0f;
        if (Input::GetInstance()->PushKey(DIK_S)) inputDirection.z -= 1.0f;
        if (Input::GetInstance()->PushKey(DIK_D)) inputDirection.x += 1.0f;
        if (Input::GetInstance()->PushKey(DIK_A)) inputDirection.x -= 1.0f;

        // 入力がない場合は早期リターン
        if (inputDirection.Length() <= kMovementInputThreshold)
        {
            return Vector3(0, 0, 0);
        }

        // camera_ が設定されている場合はカメラ基準の方向に変換
        if (camera_ != nullptr)
        {
            return GetCameraRelativeDirection(inputDirection);
        }

        // camera_ が設定されていない場合はワールド座標系での移動
        inputDirection.NormalizeSelf();
        return inputDirection;
    }

    // カメラ基準の方向を取得
    Vector3 MoveComponent::GetCameraRelativeDirection(const Vector3& inputDirection) const
    {
        if (camera_ == nullptr) return inputDirection;

        // カメラの回転を取得
        Vector3 cameraRotation = camera_->GetRotate();
        float cameraYaw = cameraRotation.y;

        // カメラのY軸回転に基づいて前方向と右方向を計算
        Vector3 cameraForward(std::sin(cameraYaw), 0.0f, std::cos(cameraYaw));
        Vector3 cameraRight(std::cos(cameraYaw), 0.0f, -std::sin(cameraYaw));

        // 入力方向をカメラ基準に変換
        Vector3 moveDirection =
            cameraForward * inputDirection.z +  // 前後入力
            cameraRight * inputDirection.x;     // 左右入力

        // Y軸は常に0に保つ（地面に沿って移動）
        moveDirection.y = 0.0f;

        // 正規化
        if (moveDirection.Length() > kMovementInputThreshold)
        {
            moveDirection.NormalizeSelf();
        }

        return moveDirection;
    }

    // マウスの方向を向く処理
    void MoveComponent::ProcessLookAtMouse(::GameObject* owner)
    {
        if (camera_ == nullptr) return;

        // マウスのスクリーン座標を取得
        float mouseX = Input::GetInstance()->GetMouseX();
        float mouseY = Input::GetInstance()->GetMouseY();

        // ビューポート行列を作成（1280x720の仮想スクリーン解像度に固定）
        Matrix4x4 matViewport = MakeViewportMatrix(0, 0, 1280.0f, 720.0f, 0.0f, 1.0f);

        // ビュー行列とプロジェクション行列を合成
        Matrix4x4 matVPV = (camera_->GetViewMatrix() * camera_->GetProjectionMatrix()) * matViewport;

        // 合成行列の逆行列を計算
        Matrix4x4 matInverseVPV = Inverse(matVPV);

        // スクリーン座標を定義（近点と遠点）
        Vector3 posNear = Vector3(mouseX, mouseY, 0.0f);
        Vector3 posFar = Vector3(mouseX, mouseY, 1.0f);

        // スクリーン座標をワールド座標に変換
        posNear = MathUtils::Transform(posNear, matInverseVPV);
        posFar = MathUtils::Transform(posFar, matInverseVPV);

        // オーナーの位置を取得
        Vector3 ownerPos = owner->GetPosition();

        // レイの方向を計算
        Vector3 rayDir = Vector3::Normalize(posFar - posNear);

        // オーナーと同じ高さの平面との交点を計算
        // rayDir.y が 0 に近い場合のゼロ除算を防ぐ
        if (std::abs(rayDir.y) > 0.0001f)
        {
            float t = (ownerPos.y - posNear.y) / rayDir.y;
            Vector3 targetPos = posNear + rayDir * t;

            // ターゲット方向を計算
            Vector3 direction = targetPos - ownerPos;
            direction.y = 0.0f; // Y軸方向のみ変更するため、高さを無視

            // 向きを補間して更新
            if (direction.Length() > kMovementInputThreshold)
            {
                UpdateRotation(owner, direction);
            }
        }
    }
}