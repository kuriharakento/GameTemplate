#pragma once
#include "application/UI/GameUI.h"

/**
 * @brief レベルアップ時の通知演出を管理するクラス。
 * 暫定的に uvChecker.png を使用して表示を行う。
 */
class LevelUpUI : public GameUI
{
public:
    /**
     * @brief 初期化処理
     * @param spriteCommon スプライト共通設定
     */
    void Initialize(SpriteCommon* spriteCommon);

    /**
     * @brief 更新処理
     */
    void Update();

    /**
     * @brief 演出を開始する
     */
    void Trigger();

    /**
     * @brief 演出中かどうかを取得
     */
    bool IsActive() const { return isActive_; }

private:
    float timer_ = 0.0f;
    bool isActive_ = false;

    // 演出時間（秒）
    static constexpr float kDuration = 2.0f;
};
