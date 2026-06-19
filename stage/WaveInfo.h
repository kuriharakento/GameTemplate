#pragma once
#include <vector>

#include "GameObjectInfo.h"

/**
 * @brief ウェーブ情報を格納する構造体
 * 
 * 1つのウェーブ（敵の出現グループ）に関する情報を保持します。
 * JSONファイルから読み込まれ、Waveクラスの生成に使用されます。
 * 
 * ステージの階層構造: Stage → Area → Wave → Enemy
 * この構造体は、Wave単位でどの敵をスポーンさせるかを定義します。
 */
struct WaveInfo
{
	std::vector<GameObjectInfo> enemies; ///< ウェーブに含まれる敵の情報リスト
};

/**
 * @brief JSON自動シリアライズマクロ
 * 
 * nlohmann/jsonライブラリによる自動変換を有効化します。
 * これにより、JSONファイルとWaveInfo構造体の相互変換が可能になります。
 */
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WaveInfo, enemies)
