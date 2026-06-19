#pragma once
// editor
#include "GameObjectInfo.h"
#include "engine/jsonEditor/JsonEditableBase.h"

/**
 * @brief ステージのゲームオブジェクト配置データをJSON連携するクラス
 * 
 * JSONファイルからステージ上のゲームオブジェクト（プレイヤー、障害物など）の
 * 配置情報を読み込み、ゲーム内で使用できる形式で保持します。
 * JsonEditableBaseを継承しており、ImGuiを使用した編集機能を持ちます。
 * 
 * 用途:
 * - プレイヤーのスポーン位置定義
 * - 障害物の配置情報
 * - その他ステージ固定オブジェクトの配置
 * 
 * 注意: 敵の配置情報は含まれません（AreaWaveDataで管理）
 * 
 * JSONファイル構造例:
 * @code
 * {
 *   "objects": [
 *     {
 *       "type": "PlayerSpawn",
 *       "name": "player",
 *       "disabled": false,
 *       "fileName": "player.obj",
 *       "transform": { "translate": [0, 0, 0], ... }
 *     },
 *     {
 *       "type": "Obstacle",
 *       ...
 *     }
 *   ]
 * }
 * @endcode
 * 
 * 使用例:
 * @code
 * // JSONファイルからステージデータを読み込み
 * auto stageData = std::make_shared<StageData>();
 * stageData->LoadJson("stage/stage1.json");
 * 
 * // オブジェクト情報をタイプごとに処理
 * for (const auto& objInfo : stageData->gameObjects) {
 *     if (objInfo.type == "PlayerSpawn") {
 *         CreatePlayer(objInfo);
 *     } else if (objInfo.type == "Obstacle") {
 *         CreateObstacle(objInfo);
 *     }
 * }
 * @endcode
 */
class StageData : public JsonEditableBase
{
public:
	/**
	 * @brief コンストラクタ
	 * 
	 * メンバ変数をJSON編集システムに登録します。
	 */
	StageData();

	/**
	 * @brief ImGuiでのデバッグ表示・編集
	 * 
	 * ゲームオブジェクトの情報を表示し、ImGuiで編集可能にします。
	 * デバッグビルド時のみ有効です（USE_IMGUIマクロで制御）。
	 * 
	 * 編集可能な項目:
	 * - 無効化フラグ（disabled）
	 * - 位置、回転、スケール
	 */
	void DrawImGui() override;

	std::vector<GameObjectInfo> gameObjects; ///< ステージ上のゲームオブジェクトのリスト（JSONから読み込まれる）
};

