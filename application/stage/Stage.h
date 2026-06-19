#pragma once
#include <memory>
#include "AreaManager.h"
#include "AreaWaveData.h"

/**
 * @brief ステージ全体を管理するクラス
 * 
 * 1つのステージ（ゲームのステージ全体）を表し、複数のエリアを管理します。
 * JSONファイルからステージ構成を読み込み、エリアマネージャーを使用して
 * エリアの進行を制御します。
 * 
 * ステージの階層構造における最上位:
 * **Stage** → AreaManager → Area → WaveManager → Wave → Enemy
 * 
 * 主な機能:
 * - JSONファイルからステージ構成（エリアとウェーブ）を読み込み
 * - AreaManagerを使用した複数エリアの管理
 * - ステージクリア判定とコールバック
 * - デバッグ用の敵スポーン位置表示
 * 
 * 使用例:
 * @code
 * // ステージの生成
 * Stage stage(objCommon, lightManager, enemyManager, "stage/stage1_area.json");
 * 
 * // ステージクリア時のコールバック設定
 * stage.SetOnClearCallback([]() {
 *     std::cout << "Stage Cleared!" << std::endl;
 * });
 * 
 * // ステージ開始
 * stage.Start();
 * 
 * // 更新処理（毎フレーム呼び出し）
 * stage.Update(cameraManager);
 * @endcode
 */
class Stage
{
public:
    /**
     * @brief コンストラクタ
     * 
     * JSONファイルからエリアとウェーブの情報を読み込み、
     * AreaManagerと各Areaオブジェクトを生成します。
     * 
     * @param object3dCommon 3Dオブジェクト共通データへのポインタ
     * @param lightManager ライトマネージャーへのポインタ
     * @param ememyManager 敵マネージャーへのポインタ
     * @param filePath エリア・ウェーブ情報のJSONファイルパス（例: "stage/stage1_area.json"）
     */
    Stage(Object3dCommon* object3dCommon, LightManager* lightManager, EnemyManager* ememyManager, const std::string& filePath);

    /**
     * @brief ステージを開始する
     * 
     * AreaManagerを起動し、最初のエリアを開始します。
     */
    void Start();

    /**
     * @brief ステージの更新処理
     * 
     * AreaManagerの更新を行います。
     * デバッグモードでは、敵のスポーン位置を可視化します。
     * 毎フレーム呼び出す必要があります。
     * 
     * @param camera カメラマネージャーへのポインタ（トランスフォーム計算とデバッグ表示に使用）
     */
    void Update(CameraManager* camera);

    /**
     * @brief ステージクリア時のコールバック関数を設定
     * 
     * このコールバックは、ステージ内の全エリアがクリアされた際に呼ばれます。
     * 次のステージへの遷移や、クリア演出の開始などに使用されます。
     * 
     * @param cb ステージクリア時に実行される関数オブジェクト
     */
    void SetOnClearCallback(std::function<void()> cb) { onClearCallback_ = std::move(cb); }

    /**
     * @brief ステージがクリアされているかを取得
     * @return true: ステージクリア済み（全エリアクリア）, false: 未クリア
     */
    bool IsCleared() const { return isCleared_; }

    /**
     * @brief AreaManagerを取得
     * @return AreaManagerへのポインタ
     */
    AreaManager* GetAreaManager() const { return areaManager_.get(); }

private:
    std::unique_ptr<AreaManager> areaManager_;  ///< エリアマネージャー（複数エリアの進行を制御）
    bool isCleared_ = false;                    ///< ステージがクリアされているか
    std::function<void()> onClearCallback_;     ///< ステージクリア時のコールバック
	std::shared_ptr<AreaWaveData> areaWaveData_; ///< エリアとウェーブの情報（JSONから読み込み）
};
