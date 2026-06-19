#pragma once
#include "engine/gameobject/component/base/IActionComponent.h"
#include "math/Vector3.h"
#include "input/Input.h"
#include "effects/particle/ParticleManager.h"

class EnemyManager;
class GameObject;
class Camera;
class PostProcessManager;
class Player;
class CameraManager;

/**
 * @brief エフェクトのイージング種類
 */
enum class EffectEasingType {
    EaseOutQuad,
    EaseInOutSine,
    EaseOutExpo,
    EffectEasingType_EaseOutBack
};

namespace GameObjectComponent
{
    /**
     * @brief プレイヤーの移動と回避を制御するコンポーネント
     *
     * WASD移動、カメラ基準の方向変換、回避（ダッジ）、バレットタイム機能を提供する
     */
    class MoveComponent : public IActionComponent
    {
    public:
        /**
         * @brief コンストラクタ
         * @param enemyManager 敵マネージャー（バレットタイム判定用）
         * @param camera カメラマネージャー
         * @param postProcessManager ポストプロセスマネージャー（グレースケール用）
         */
        MoveComponent(EnemyManager* enemyManager, CameraManager* camera, PostProcessManager* postProcessManager = nullptr);

        /**
         * @brief フレームごとの更新処理
         * @param owner このコンポーネントを所有するゲームオブジェクト
         */
        void Update(::GameObject* owner) override;

        /**
         * @brief 回避速度を設定する
         * @param speed 回避速度
         */
        void SetDodgeSpeed(float speed) { dodgeSpeed_ = speed; }

        /**
         * @brief 回避持続時間を設定する
         * @param duration 回避持続時間（秒）
         */
        void SetDodgeDuration(float duration) { dodgeDuration_ = duration; }

        /**
         * @brief 回避クールダウンを設定する
         * @param cooldown クールダウン時間（秒）
         */
        void SetDodgeCooldown(float cooldown) { dodgeCooldown_ = cooldown; }

        /**
         * @brief 回避中の無敵時間を設定する
         * @param time 無敵時間（秒）
         */
        void SetDodgeInvincibleTime(float time) { dodgeInvincibleTime_ = time; }

        /**
         * @brief 回避距離を設定する
         * @param distance 回避距離
         */
        void SetDodgeDistance(float distance) { dodgeDistance_ = distance; }

        /**
         * @brief 回転補間速度を設定する
         * @param speed 回転補間速度
         */
        void SetRotationSpeed(float speed) { rotationSpeed_ = speed; }

        /**
         * @brief 回避中かどうかを取得する
         * @return 回避中ならtrue
         */
        bool IsDodging() const { return dodgeTimer_ > 0.0f; }

        /**
         * @brief 無敵状態かどうかを取得する
         * @return 無敵状態ならtrue
         */
        bool IsInvincible() const { return invincibleTimer_ > 0.0f; }

        /**
         * @brief 移動入力があるかどうかを取得する
         * @return 移動入力があればtrue
         */
        bool IsMoving() const { return hasMovementInput_; }

        /**
         * @brief 歩行アニメーションのインデックスを設定する
         * @param index アニメーションインデックス
         */
        void SetWalkAnimationIndex(uint32_t index) { walkAnimationIndex_ = index; }

        /**
         * @brief 回避動作の進行度を取得する
         * @return 進行度（0.0〜1.0）
         */
        float GetDodgeProgress() const;

        /**
         * @brief バレットタイム中かどうかを取得する
         * @return バレットタイム中ならtrue
         */
        bool IsInBulletTime() const { return isInBulletTime_; }

        /**
         * @brief バレットタイムのクールダウン中かどうかを取得する
         * @return クールダウン中ならtrue
         */
        bool IsBulletTimeCoolingDown() const;

        /**
         * @brief バレットタイムクールダウンの進行度を取得する
         * @return 進行度（0.0〜1.0）
         */
        float GetBulletTimeCooldownProgress() const;

    private:
        // 定数
        // デフォルト回転補間速度
        static constexpr float kDefaultRotationSpeed = 0.1f;
        // デフォルト回避速度
        static constexpr float kDefaultDodgeSpeed = 30.0f;
        // デフォルト回避持続時間
        static constexpr float kDefaultDodgeDuration = 0.25f;
        // デフォルト回避クールダウン
        static constexpr float kDefaultDodgeCooldown = 0.8f;
        // デフォルト回避無敵時間
        static constexpr float kDefaultDodgeInvincibleTime = 0.25f;
        // デフォルト回避距離
        static constexpr float kDefaultDodgeDistance = 8.0f;
        // 回避開始時の初速倍率
        static constexpr float kDodgeImpulse = 1.5f;
        // バレットタイム範囲
        static constexpr float kBulletTimeRadius = 5.0f;
        // バレットタイムのスローモーション倍率
        static constexpr float kBulletTimeScale = 0.2f;
        // バレットタイムの持続時間
        static constexpr float kBulletTimeDuration = 3.0f;
        // バレットタイムのクールダウン時間
        static constexpr float kBulletTimeCooldown = 5.0f;
        // エフェクト間隔
        static constexpr float kEffectInterval = 0.03f;
        // 移動入力の閾値
        static constexpr float kMovementInputThreshold = 0.01f;
        // 回避中の回転補間速度
        static constexpr float kDodgeRotationInterpolation = 0.2f;
        // 完全なタイムスケール（通常速度）
        static constexpr float kNormalTimeScale = 1.0f;

        // 歩行アニメーションのインデックス
        uint32_t walkAnimationIndex_ = 0;

        // 移動処理
        void ProcessMovement(::GameObject* owner, float deltaTime);
        // 回避処理
        void ProcessDodge(::GameObject* owner);
        // 移動方向を取得
        Vector3 GetMovementDirection() const;
        // カメラ基準の方向を取得
        Vector3 GetCameraRelativeDirection(const Vector3& inputDirection) const;
        // 回避エフェクトを再生
        void PlayDodgeEffect(::GameObject* owner);
        // 向き補間処理
        void UpdateRotation(::GameObject* owner, const Vector3& direction);
        // マウスの方向を向く処理
        void ProcessLookAtMouse(::GameObject* owner);
        // バレットタイム処理
        void ProcessBulletTime(::GameObject* owner);
        // バレットタイム発動
        void ActivateBulletTime(::GameObject* owner);
        // バレットタイム中のバフを付与
        void ApplyBulletTimeBuffs();
        // バレットタイム中のバフを解除
        void RemoveBulletTimeBuffs();

    private:
        // 敵マネージャー
        EnemyManager* enemyManager_ = nullptr;
        // カメラ
        Camera* camera_ = nullptr;
        // ポストプロセスマネージャー
        PostProcessManager* postProcessManager_ = nullptr;
        // バレットタイムバフ付与のためのプレイヤー参照（所有しない）
        Player* player_ = nullptr;
        // 軌跡パーティクルのポインタ
        ParticleEffect* trailEffect_ = nullptr;

        // 移動速度
        float moveSpeed_ = 0.0f;

        // 回転補間速度
        float rotationSpeed_ = kDefaultRotationSpeed;

        // 回避中の移動速度
        float dodgeSpeed_ = kDefaultDodgeSpeed;
        // 回避動作の持続時間
        float dodgeDuration_ = kDefaultDodgeDuration;
        // 回避のクールダウン時間
        float dodgeCooldown_ = kDefaultDodgeCooldown;
        // 回避中の無敵時間
        float dodgeInvincibleTime_ = kDefaultDodgeInvincibleTime;
        // 回避距離
        float dodgeDistance_ = kDefaultDodgeDistance;
        // 回避開始時の初速倍率
        float dodgeImpulse_ = kDodgeImpulse;
        // 回避タイマー
        float dodgeTimer_ = 0.0f;
        // 回避クールダウンタイマー
        float dodgeCooldownTimer_ = 0.0f;
        // 無敵タイマー
        float invincibleTimer_ = 0.0f;
        // 回避方向
        Vector3 dodgeDirection_;
        // 回避開始位置
        Vector3 dodgeStartPosition_;
        // 回避目標位置
        Vector3 dodgeTargetPosition_;
        // 移動入力があるか
        bool hasMovementInput_ = false;
        // 回避中か
        bool isDodging_ = false;
        // バレットタイム範囲
        float bulletTimeRadius_ = kBulletTimeRadius;
        // バレットタイム中か
        bool isInBulletTime_ = false;
        // バレットタイムのスローモーション倍率
        float bulletTimeScale_ = kBulletTimeScale;
        // バレットタイムの持続時間
        float bulletTimeDuration_ = kBulletTimeDuration;
        // バレットタイムのクールダウン時間
        float bulletTimeCooldown_ = kBulletTimeCooldown;
        //　射撃レートバフの倍率
        float fireRateBuff_ = 0.2f;
        // 移動速度バフの倍率
        float moveSpeedBuff_ = 0.8f;

        // エフェクトタイマー
        float effectTimer_ = 0.0f;
        // エフェクトの強度（0.0f = オフ, 1.0f = オン）
        float effectIntensity_ = 0.0f;
        // エフェクト移行の進行度（0.0f ～ 1.0f）
        float effectTransitionProgress_ = 0.0f;
        // エフェクト移行にかける時間（秒）
        float effectTransitionDuration_ = 0.2f;
        // 選択中のイージング関数
        EffectEasingType effectEasingType_ = EffectEasingType::EaseOutQuad;
        // 残像間隔
        float effectInterval_ = kEffectInterval;


        // 回避の最初のフレームか
        bool isFirstDodgeFrame_ = false;
        // エフェクト再生済みか
        bool wasEffectPlayed_ = false;
    };
}