#include "ScoreManager.h"

// 静的メンバ変数の実体化
uint64_t ScoreManager::score_ = 0;

void ScoreManager::Reset()
{
    score_ = 0;
}

void ScoreManager::AddScore(uint64_t amount)
{
    score_ += amount;
}

uint64_t ScoreManager::GetScore()
{
    return score_;
}
