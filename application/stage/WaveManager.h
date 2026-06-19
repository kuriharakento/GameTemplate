#pragma once
#include <vector>

#include "Wave.h"
class EnemyManager;

/**
 * @brief 複数のウェーブを順番に管理するウェーブシーケンス管理クラス
 * 
 * エリア内の複数のウェーブを順番に実行し、ウェーブ間の待機時間を制御します。
 * 全てのウェーブがクリアされた際の処理も管理します。
 * 
 * ステージの階層構造における位置:
 * Stage → Area → **WaveManager** → Wave → Enemy
 * 
 * 主な機能:
 * - ウェーブの順次実行制御
 * - ウェーブ間の待機時間管理（演出用）
 * - 全ウェーブクリア判定とコールバック
 * - デバッグ用のウェーブスキップ機能
 * 
 * 使用例:
 * @code
 * // ウェーブマネージャーの生成と初期化
 * std::vector<Wave> waves = CreateWavesFromJSON();
 * WaveManager waveManager(enemyManager, waves);
 * 
 * // 全ウェーブクリア時のコールバック設定
 * waveManager.SetOnAllWavesCleared([]() {
 *     std::cout << "All Waves Cleared!" << std::endl;
 * });
 * 
 * // ウェーブシーケンス開始
 * waveManager.Start();
 * 
 * // 更新処理（毎フレーム呼び出し）
 * waveManager.Update();
 * @endcode
 */
class WaveManager
{
public:
	/**
	 * @brief コンストラクタ
	 * @param enemyManager 敵を管理するEnemyManagerへのポインタ
	 * @param waves 順番に実行するウェーブのリスト
	 */
	WaveManager(EnemyManager* enemyManager, const std::vector<Wave>& waves);

	/**
	 * @brief ウェーブマネージャーの更新処理
	 * 
	 * ウェーブ間の待機時間を管理し、待機時間が終了したら次のウェーブを開始します。
	 * 毎フレーム呼び出す必要があります。
	 */
	void Update();

	/**
	 * @brief 最初のウェーブを開始する
	 * 
	 * ウェーブシーケンスの開始点です。
	 * ウェーブが存在しない場合は何もしません。
	 */
	void Start();

	/**
	 * @brief 全ウェーブクリア時のコールバック関数を設定
	 * 
	 * このコールバックは、全てのウェーブがクリアされた際に呼ばれます。
	 * エリアクリアの通知などに使用されます。
	 * 
	 * @param callback 全ウェーブクリア時に実行される関数オブジェクト
	 */
	void SetOnAllWavesCleared(std::function<void()> callback) { onAllWavesCleared_ = std::move(callback); }

	/**
	 * @brief 全てのウェーブがクリアされているかを取得
	 * @return true: 全ウェーブクリア済み, false: 未クリア
	 */
	bool IsAllCleared() const { return currentWaveIndex_ >= static_cast<int>(waves_.size()); }

	/**
	 * @brief 現在のウェーブを強制的にスキップして次のウェーブへ進む
	 * 
	 * デバッグやテスト用の機能です。
	 * 待機時間をキャンセルして即座に次のウェーブを開始します。
	 */
	void SkipToNextWave();

private:
	/**
	 * @brief 現在のウェーブを開始する内部処理
	 * 
	 * 現在のインデックスに対応するウェーブを開始し、
	 * そのウェーブのクリアコールバックに次のウェーブへの遷移処理を設定します。
	 * 全ウェーブが終了している場合は、全クリアコールバックを実行します。
	 */
	void StartCurrentWave();

private:
	EnemyManager* enemyManager_;     ///< 敵マネージャーへのポインタ
	std::vector<Wave> waves_;        ///< ウェーブのリスト
	int currentWaveIndex_;           ///< 現在実行中のウェーブのインデックス
	std::function<void()> onAllWavesCleared_; ///< すべてのウェーブクリア時のコールバック

	// ウェーブ間ウェイト用
	bool waitForNextWave_;           ///< 次のウェーブ開始を待機中かどうか
	float waitTimer_;                ///< ウェーブ間の待機タイマー（秒単位）
};

