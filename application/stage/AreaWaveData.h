#pragma once
#include <vector>

#include "AreaInfo.h"
#include "jsonEditor/JsonEditableBase.h"

/**
 * @brief エリアとウェーブのデータをJSON連携するクラス
 * 
 * JSONファイルから複数エリアとそれぞれのウェーブ情報を読み込み、
 * ゲーム内で使用できる形式で保持します。
 * JsonEditableBaseを継承しており、ImGuiを使用した編集機能を持ちます。
 * 
 * JSONファイル構造例:
 * @code
 * {
 *   "areas": [
 *     {
 *       "areaIndex": 0,
 *       "areaTransform": { "translate": [0, 0, 0], ... },
 *       "waves": [
 *         { "enemies": [...] }
 *       ]
 *     }
 *   ]
 * }
 * @endcode
 * 
 * 使用例:
 * @code
 * // JSONファイルからデータを読み込み
 * auto areaWaveData = std::make_shared<AreaWaveData>();
 * areaWaveData->LoadJson("stage/stage1_area.json");
 * 
 * // エリア情報を取得してゲームオブジェクトを生成
 * for (const auto& areaInfo : areaWaveData->areas) {
 *     CreateArea(areaInfo);
 * }
 * @endcode
 */
class AreaWaveData : public JsonEditableBase
{
public:
	/**
	 * @brief コンストラクタ
	 * 
	 * メンバ変数をJSON編集システムに登録します。
	 */
	AreaWaveData();

    /**
     * @brief ImGuiでのデバッグ表示
     * 
     * エリア、ウェーブ、敵の情報を階層的に表示します。
     * デバッグビルド時のみ有効です（USE_IMGUIマクロで制御）。
     * 
     * 表示内容:
     * - 各エリアのインデックスとウェーブ数
     * - 各ウェーブの敵情報（名前、タイプ、位置など）
     */
    void DrawImGui() override;

	std::vector<AreaInfo> areas; ///< 全エリアの情報リスト（JSONから読み込まれる）
};

