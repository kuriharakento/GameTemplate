#pragma once
#include "graphics/2d/NumberSprite.h"

/**
 * @brief コンボシステム管理クラス（シングルトン）
 * 
 * 敵撃破時のコンボ数管理とタイマー制御を行います。
 * 連続して敵を倒すことでコンボが継続し、時間内に次の敵を倒せないとリセットされます。
 * コンボ数はカーネージモードの発動条件にも使用されます。
 * 
 * @note シングルトンパターンで実装されており、ゲーム全体で一つのインスタンスのみ存在します
 * 
 * @code
 * // 敵を倒した時の使用例
 * ComboManager::GetInstance().OnEnemyDefeated(1);
 * 
 * // コンボ数の取得
 * int currentCombo = ComboManager::GetInstance().GetComboCount();
 * 
 * // コンボのリセット（プレイヤー死亡時など）
 * ComboManager::GetInstance().Reset();
 * @endcode
 */
class ComboManager
{
public:
    ~ComboManager();

#ifdef USE_IMGUI
    void DrawImGui();
#endif
    /**
     * @brief シングルトンインスタンスの取得
     * 
     * @return ComboManager& グローバルなComboManagerインスタンスへの参照
     */
    static ComboManager& GetInstance()
    {
        static ComboManager instance;
        return instance;
    }

    /**
     * @brief コンボシステムの初期化
     * 
     * コンボ数表示用のスプライトを初期化します。
     * シーン開始時に一度だけ呼び出してください。
     * 
     * @param spriteCommon スプライト共通設定へのポインタ
     */
    void Initialize(SpriteCommon* spriteCommon);

    /**
     * @brief 敵撃破時の処理
     * 
     * コンボ数を増加させ、タイマーをリセットします。
     * 
     * @param count 撃破した敵の数（デフォルト: 1）
     */
    void OnEnemyDefeated(int count = 1);

    /**
     * @brief 毎フレームの更新処理
     * 
     * コンボタイマーをデクリメントし、タイムアウト時にコンボをリセットします。
     * ImGuiデバッグ表示も行います。
     */
    void Update();

    /**
     * @brief コンボ数の描画
     * 
     * 画面上にコンボ数を表示します。コンボが0の場合は描画されません。
     */
    void Draw();

    /**
     * @brief コンボの強制リセット
     * 
     * コンボ数とタイマーを0にリセットします。
     * エリア遷移、プレイヤー死亡時などに使用します。
     */
    void Reset();

    // ==================================================
    // ゲッター
    // ==================================================
    
    /**
     * @brief 現在のコンボ数を取得
     * 
     * @return int 現在のコンボ数
     */
    int GetComboCount() const { return comboCount_; }
    
    /**
     * @brief コンボタイマーの残り時間を取得
     * 
     * @return float コンボタイマーの残り時間（秒）
     */
    float GetComboTimer() const { return comboTimer_; }
    
    /**
     * @brief コンボがアクティブかどうかを判定
     * 
     * @return bool コンボ数が1以上の場合true
     */
    bool IsActive() const { return comboCount_ > 0; }

    // コンボの猶予時間（秒）
    static constexpr float kComboTimeout = 10.0f;

private:


private:
    // 最大コンボ数（統計用）
    int maxComboCount_ = 0;
    // 現在のコンボ数
    int comboCount_ = 0;
    // コンボ猶予タイマー（秒）
    float comboTimer_ = 0.0f;
    // コンボ数表示用スプライト
    NumberSprite comboNumberSprite_;

private: // シングルトン
    ComboManager() = default;
    ComboManager(const ComboManager&) = delete;
    ComboManager& operator=(const ComboManager&) = delete;
};
