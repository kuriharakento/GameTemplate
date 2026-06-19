#pragma once
// json
#include <nlohmann/json.hpp>
// system
#include "base/GraphicsTypes.h"

/**
 * @brief JSONから読み込むゲームオブジェクトの情報を格納する構造体
 * 
 * この構造体は、ステージやウェーブ管理システムにおいて、
 * プレイヤー、敵、障害物などのゲームオブジェクトの配置情報を保持します。
 * JSONファイルから読み込まれ、実際のゲームオブジェクト生成の基礎データとして使用されます。
 * 
 * 使用例:
 * @code
 * // JSONファイルから読み込まれた情報を基にオブジェクトを配置
 * for (const auto& objInfo : waveInfo.enemies) {
 *     if (objInfo.type == "Enemy") {
 *         CreateEnemy(objInfo);
 *     }
 * }
 * @endcode
 */
struct GameObjectInfo
{
	std::string type;		///< ゲームオブジェクトのタイプ（"Player", "Enemy", "Obstacle"など）
	std::string name;		///< モデル名（識別用の名前）
	bool disabled;			///< 無効化フラグ（trueの場合、このオブジェクトは生成されない）
	std::string fileName;	///< モデルファイル名（読み込むモデルのファイルパス）
	Transform transform;	///< トランスフォーム情報（位置、回転、スケール）
};

/**
 * @brief JSON自動シリアライズマクロ
 * 
 * nlohmann/jsonライブラリのマクロを使用して、GameObjectInfo構造体とJSONの相互変換を自動生成します。
 * これにより、JSONファイルから構造体への読み込みと、構造体からJSONファイルへの書き出しが簡単に行えます。
 */
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GameObjectInfo, type, name, disabled, fileName, transform)
