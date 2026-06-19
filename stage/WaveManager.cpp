#include "WaveManager.h"

#include "time/TimeManager.h"

WaveManager::WaveManager(EnemyManager* enemyManager, const std::vector<Wave>& waves)
{
	enemyManager_ = enemyManager;  // 敵マネージャーのポインタを保存
	waves_ = waves;                // ウェーブリストをコピー
	currentWaveIndex_ = 0;         // 最初のウェーブから開始
	onAllWavesCleared_ = nullptr;  // コールバック未設定
	waitForNextWave_ = false;      // 待機状態ではない
	waitTimer_ = 0.0f;             // タイマー初期化
}

void WaveManager::Update()
{
	// ウェーブ間の待機処理
	if (waitForNextWave_)
	{
		// デルタタイムを減算してタイマーを進める
		waitTimer_ -= TimeManager::GetInstance().GetGameContext().deltaTime;
		if (waitTimer_ <= 0.0f)
		{
			// 待機時間終了：次のウェーブを開始
			waitForNextWave_ = false;
			++currentWaveIndex_;
			StartCurrentWave();
		}
	}

}

void WaveManager::SkipToNextWave()
{
	// 待機をキャンセルして次のウェーブを即座に開始（デバッグ用）
	waitForNextWave_ = false;
	waitTimer_ = 0.0f;
	++currentWaveIndex_;
	StartCurrentWave();
}

void WaveManager::StartCurrentWave()
{
	// 全ウェーブ終了チェック
	if (currentWaveIndex_ >= waves_.size())
	{
		// 全ウェーブクリア時のコールバックを実行
		if (onAllWavesCleared_) onAllWavesCleared_();
		return;
	}

	// 現在のウェーブにクリアコールバックを設定
	// ウェーブクリア時に待機状態に入る
	waves_[currentWaveIndex_].SetOnClearCallback([this]() {
		waitForNextWave_ = true;  // 次のウェーブまで待機
		waitTimer_ = 2.0f;        // 2秒間の待機時間を設定
												 });

	// ウェーブを開始（敵をスポーン）
	waves_[currentWaveIndex_].Start(enemyManager_);
}

void WaveManager::Start()
{
	// ウェーブが存在しない場合は早期リターン
	if (waves_.empty()) { return; }

	// 最初のウェーブを開始
	StartCurrentWave();
}

