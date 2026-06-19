#pragma once
#include <vector>
#include <functional>
#include "Area.h"
#include <effects/particle/ParticleManager.h>

/**
 * @brief 複数のエリアを順番に管理するエリア管理クラス
 * 
 * ステージ内の複数のエリアを順次進行させ、全エリアクリア時の処理を管理します。
 * プレイヤーがエリアをクリアすると自動的に次のエリアがアクティブになります。
 * 
 * ステージの階層構造における位置:
 * Stage → **AreaManager** → Area → WaveManager → Wave → Enemy
 * 
 * 主な機能:
 * - エリアの順次進行制御（1つずつ有効化）
 * - 全エリアクリア判定とコールバック
 * - 現在のアクティブエリアの管理
 * - エリア開始時のコールバック（演出制御用）
 * - エリアエフェクトの制御
 * 
 * 使用例:
 * @code
 * // エリアマネージャーの生成
 * std::vector<std::shared_ptr<Area>> areas = CreateAreasFromJSON();
 * AreaManager areaManager(areas);
 * 
 * // 全エリアクリア時のコールバック設定
 * areaManager.SetOnAllAreasCleared([]() {
 *     std::cout << "All Areas Cleared!" << std::endl;
 * });
 * 
 * // エリア開始時のコールバック設定（演出用）
 * areaManager.SetOnAreaStarted([](int index, Area* area) {
 *     std::cout << "Area " << index << " Started!" << std::endl;
 * });
 * 
 * // エリアマネージャー開始
 * areaManager.Start();
 * 
 * // 更新処理（毎フレーム呼び出し）
 * areaManager.Update(cameraManager);
 * @endcode
 */
class AreaManager
{
public:
    /**
     * @brief コンストラクタ
     * @param areas 管理する全エリアのリスト（順番に実行される）
     */
    AreaManager(const std::vector<std::shared_ptr<Area>>& areas);

    /**
     * @brief エリアマネージャーを開始する
     * 
     * 最初のエリアをアクティブにして、エリアシーケンスを開始します。
     * エリアが存在しない場合は何もしません。
     */
    void Start();

    /**
     * @brief エリアマネージャーの更新処理
     * 
     * 現在アクティブなエリアの更新を行います。
     * 毎フレーム呼び出す必要があります。
     * 
     * @param camera カメラマネージャーへのポインタ（トランスフォーム計算に使用）
     */
    void Update(CameraManager* camera);

    /**
     * @brief 全エリアクリア時のコールバック関数を設定
     * 
     * このコールバックは、全てのエリアがクリアされた際に呼ばれます。
     * ステージクリアの通知などに使用されます。
     * 
     * @param cb 全エリアクリア時に実行される関数オブジェクト
     */
    void SetOnAllAreasCleared(std::function<void()> cb) { onAllAreasCleared_ = std::move(cb); }

    /**
     * @brief 全エリアがクリアされているかを取得
     * @return true: 全エリアクリア済み, false: 未クリア
     */
    bool IsAllCleared() const { return isAllCleared_; }

    /**
     * @brief 現在アクティブなエリアのインデックスを取得
     * @return 現在のエリアインデックス（0から開始）
     */
    int CurrentAreaIndex() const { return currentAreaIndex_; }

    /**
     * @brief 現在アクティブなエリアを取得
     * @return 現在のエリアへのポインタ（全エリアクリア後はnullptr）
     */
    Area* GetCurrentArea() const { return (currentAreaIndex_ < areas_.size()) ? areas_[currentAreaIndex_].get() : nullptr; }

    /**
     * @brief 全エリアのリストを取得
     * @return エリアリストへの参照
     */
    const std::vector<std::shared_ptr<Area>>& GetAreas() const { return areas_; }

    /**
     * @brief エリア開始時のコールバック関数を設定
     * 
     * このコールバックは、新しいエリアが開始される際に呼ばれます。
     * エリア演出の開始や、UIの更新などに使用されます。
     * 
     * @param cb エリア開始時に実行される関数オブジェクト（引数: エリアインデックス, エリアポインタ）
     */
    void SetOnAreaStarted(std::function<void(int, Area*)> cb) { onAreaStarted_ = std::move(cb); }

private:
    /**
     * @brief 現在のエリアを開始する内部処理
     * 
     * 現在のインデックスに対応するエリアをアクティブにし、
     * そのエリアのクリアコールバックに次のエリアへの遷移処理を設定します。
     * 全エリアが終了している場合は、全クリアコールバックを実行します。
     */
    void StartCurrentArea();

private:
	std::vector<std::shared_ptr<Area>> areas_; ///< 管理するエリアのリスト
	int currentAreaIndex_;                     ///< 現在アクティブなエリアのインデックス
	bool isAllCleared_;                        ///< 全エリアがクリアされているか
	std::function<void()> onAllAreasCleared_;  ///< 全エリアクリア時のコールバック
	std::function<void(int, Area*)> onAreaStarted_; ///< エリア開始時のコールバック
	ParticleEffect* areaEffect_;                     ///< エリアエフェクト
};