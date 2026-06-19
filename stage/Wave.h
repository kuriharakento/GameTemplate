#pragma once
#include <vector>

#include "GameObjectInfo.h"

class EnemyManager;

/**
 * @brief ウェーブ（敵の出現グループ）を管理するクラス
 * 
 * 1つのウェーブは複数の敵で構成され、すべての敵が倒されるとクリアとなります。
 * ゲームの進行に合わせて段階的に敵を出現させる機能を提供します。
 * 
 * ステージの階層構造における位置:
 * Stage → Area → **Wave** → Enemy
 * 
 * 主な機能:
 * - ウェーブ開始時に敵をEnemyManagerに追加
 * - 敵全滅時のクリア判定とコールバック実行
 * - ウェーブの状態管理（未開始/進行中/クリア済み）
 * 
 * 使用例:
 * @code
 * // ウェーブの生成
 * std::vector<GameObjectInfo> enemies = LoadEnemiesFromJSON();
 * Wave wave(enemies);
 * 
 * // クリア時のコールバック設定
 * wave.SetOnClearCallback([]() {
 *     std::cout << "Wave Cleared!" << std::endl;
 * });
 * 
 * // ウェーブ開始
 * wave.Start(enemyManager);
 * @endcode
 */
class Wave
{
public:
	/**
	 * @brief コンストラクタ
	 * @param enemies このウェーブでスポーンする敵の情報リスト
	 */
	Wave(const std::vector<GameObjectInfo>& enemies);

	/**
	 * @brief ウェーブを開始する
	 * 
	 * 敵の情報をEnemyManagerに渡して敵を生成し、
	 * 全滅時のコールバックを設定します。
	 * 既に開始されている場合は何もしません（冪等性を保証）。
	 * 
	 * @param enemyManager 敵を管理するEnemyManagerへのポインタ
	 */
	void Start(EnemyManager* enemyManager);

	/**
	 * @brief ウェーブが開始されているかを取得
	 * @return true: 開始済み, false: 未開始
	 */
	bool IsStart() const { return isStart_; }

	/**
	 * @brief ウェーブがクリアされているかを取得
	 * @return true: クリア済み（敵が全滅）, false: 未クリア
	 */
	bool IsClear() const { return isClear_; }

	/**
	 * @brief ウェーブクリア時のコールバック関数を設定
	 * 
	 * このコールバックは、ウェーブ内の全ての敵が倒された際に一度だけ呼ばれます。
	 * 次のウェーブへの遷移や演出の開始などに使用されます。
	 * 
	 * @param callback クリア時に実行される関数オブジェクト
	 */
	void SetOnClearCallback(std::function<void()> callback) { onClearCallback_ = std::move(callback); }

private:
	std::vector<GameObjectInfo> enemies_;	///< ウェーブ中にスポーンする敵の情報リスト

	bool isStart_ = false;	///< ウェーブが開始されているかどうか
	bool isClear_ = false;	///< ウェーブがクリアされているかどうか（全敵撃破）

	std::function<void()> onClearCallback_ = nullptr; ///< クリア時のコールバック関数
};

