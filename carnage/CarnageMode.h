#pragma once
#include <memory>

#include "time/Timer.h"

#include "engine/ecs/Registry.h"
#include "engine/ecs/Entity.h"

/**
 * @brief カーネージモード（特殊能力強化システム）
 * 
 * 一定数のコンボを達成することで発動する、プレイヤーの能力を一時的に強化するバフシステムです。
 * 発動中は攻撃力と移動速度が上昇し、専用のビジュアルエフェクトが表示されます。
 * 敵を倒し続けることでタイマーを延長できます。
 * 
 * @note コンボ数がcomboThreshold_（デフォルト10）に達すると自動的に発動します
 * @note タイマーが切れると通常状態に戻ります
 * 
 * @code
 * // 毎フレーム更新
 * carnageMode_->Update();
 * 
 * // 発動チェック（コンボ監視）
 * carnageMode_->TryStart();
 * 
 * // タイマー延長（敵撃破時など）
 * if (carnageMode_->IsActive()) {
 *     carnageMode_->ExtendTimer();
 * }
 * @endcode
 */
class CarnageMode
{
public:
    /**
     * @brief コンストラクタ
     * @param registry Registryへのポインタ
     * @param playerEntity プレイヤーのEntityID
     */
    CarnageMode(Registry* registry, EntityID playerEntity);
    virtual ~CarnageMode();

#ifdef USE_IMGUI
    void DrawImGui();
#endif

    /**
     * @brief 毎フレームの更新処理
     * 
     * タイマー更新、エフェクト更新、ImGuiデバッグ表示を行います。
     * タイムアウト時には自動的にバフを解除します。
     */
    void Update();

    /**
     * @brief カーネージモードの発動試行
     * 
     * 現在のコンボ数を監視し、条件を満たしていればカーネージモードを開始します。
     * 既に発動中の場合は何もしません。
     */
    void TryStart();

    /**
     * @brief タイマーの延長
     * 
     * カーネージモード中に敵を倒した際に呼び出され、タイマーを延長します。
     * 延長時間はextensionTime_（デフォルト1秒）で設定されます。
     */
    void ExtendTimer();

    /**
     * @brief カーネージモードがアクティブかどうかを判定
     * 
     * @return bool カーネージモード発動中の場合true
     */
    bool IsActive() const;
    
    /**
     * @brief 残り時間の取得
     * 
     * @return float カーネージモードの残り時間（秒）、非アクティブ時は0.0f
     */
    float GetTimeLeft() const;

private: // メンバ関数
	/**
	 * @brief バフの適用
	 * 
	 * プレイヤーに攻撃力・移動速度上昇バフを適用します。
	 * カーネージモード開始時に呼び出されます。
	 */
    void ApplyBuffs();
    
    /**
     * @brief バフの解除
     * 
     * プレイヤーから攻撃力・移動速度上昇バフを解除します。
     * カーネージモード終了時に呼び出されます。
     */
    void RemoveBuffs();
    
	/**
	 * @brief UIの表示
	 * 
	 * カーネージモード専用のUI要素を表示します。
	 */
    void ShowUI();
    
    /**
     * @brief UIの非表示
     * 
     * カーネージモード専用のUI要素を非表示にします。
     */
    void HideUI();
    
    // ECS Registry
    Registry* registry_ = nullptr;
    // ターゲットEntityID
    EntityID playerEntity_ = kInvalidEntity;
    
    // カーネージモード持続時間管理タイマー
    std::unique_ptr<Timer> timer_;

    // カーネージモード発動に必要なコンボ数
    const int comboThreshold_ = 10;
    // カーネージモード初期持続時間（秒）
    const float initialTime_ = 8.0f;
    // 敵撃破時のタイマー延長時間（秒）
    const float extensionTime_ = 1.0f;
    // 攻撃力上昇率（0.5 = 50%アップ）
    float attackUpRate_ = 0.5f;
    // 移動速度上昇率（1.0 = 100%アップ、つまり2倍速）
	float speedUpRate_ = 1.0f;
};