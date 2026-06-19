#pragma once
#include "WaveInfo.h"

/**
 * @brief エリア情報を格納する構造体
 * 
 * 1つのエリア（ステージ内の区画）に関する情報を保持します。
 * JSONファイルから読み込まれ、Areaクラスの生成に使用されます。
 * 
 * ステージの階層構造: Stage → Area → Wave → Enemy
 * この構造体は、エリアの位置・範囲と、そのエリア内で発生する複数のウェーブ情報を定義します。
 * 
 * 使用例:
 * @code
 * // JSONから読み込んだエリア情報を使用してAreaオブジェクトを生成
 * for (const auto& areaInfo : areaWaveData->areas) {
 *     auto area = CreateArea(areaInfo);
 *     area->SetPosition(areaInfo.areaTransform.translate);
 * }
 * @endcode
 */
struct AreaInfo
{
    int areaIndex;              ///< エリアのインデックス（エリアの識別番号）
    Transform areaTransform;    ///< エリアのトランスフォーム（ワールド座標での位置、回転、スケール）
    std::vector<WaveInfo> waves;///< このエリアで発生するウェーブ情報のリスト
};

/**
 * @brief JSON自動シリアライズマクロ
 * 
 * nlohmann/jsonライブラリによる自動変換を有効化します。
 * これにより、JSONファイルとAreaInfo構造体の相互変換が可能になります。
 */
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AreaInfo, areaIndex, areaTransform, waves);