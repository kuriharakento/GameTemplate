#include "Wave.h"
#include "application/gameObject/combatable/character/enemy/EnemyManager.h"

Wave::Wave(const std::vector<GameObjectInfo>& enemies)
{
	enemies_ = enemies;           // ウェーブでスポーンする敵情報を保存
	isStart_ = false;             // 未開始状態
	isClear_ = false;             // 未クリア状態
	onClearCallback_ = nullptr;   // コールバック未設定
}

void Wave::Start(EnemyManager* enemyManager)
{
	// 冪等性の保証：既に開始されている場合は何もしない
	if (isStart_) { return; }

	// ウェーブを開始状態にする
	isStart_ = true;

	// 敵全滅時のコールバックをEnemyManagerに設定
	// 全ての敵が倒されたらこのウェーブをクリアとする
	enemyManager->SetOnAllEnemiesDefeatedCallback([this]() {
		if (!isClear_)
		{
			isClear_ = true;
			if (onClearCallback_)
			{
				// ウェーブクリア時のコールバックを実行
				onClearCallback_(); 
				// コールバックは一度だけ実行されるようにnullptrに設定
				onClearCallback_ = nullptr;
			}
		}
												  });

	// 敵情報をもとにEnemyManagerに敵を追加（スポーン）
	enemyManager->AddEnemiesFromGameObjectInfo(enemies_);
}
