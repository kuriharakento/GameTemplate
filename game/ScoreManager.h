#pragma once

#include <cstdint>

/**
 * @brief ゲームプレイ中のスコアを管理するクラス
 */
class ScoreManager
{
public:
    /**
     * @brief スコアをリセットする
     */
    static void Reset();

    /**
     * @brief スコアを加算する
     * @param amount 加算量
     */
    static void AddScore(uint64_t amount);

    /**
     * @brief 現在のスコアを取得する
     * @return 現在のスコア
     */
    static uint64_t GetScore();

private:
    // コンストラクタを隠蔽
    ScoreManager() = delete;

private:
    // 現在の合計スコア
    static uint64_t score_;
};
