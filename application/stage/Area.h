#pragma once
#include <vector>
#include <functional>
#include "WaveManager.h"
#include "engine/gameobject/base/GameObject.h"

class EnemyManager;

/**
 * @brief エリア（ステージ内の区画）を管理するクラス
 * 
 * ステージ内の1つのエリアを表し、プレイヤーがエリアに侵入すると自動的に開始されます。
 * エリア内では複数のウェーブが順番に発生し、全てのウェーブをクリアするとエリアクリアとなります。
 * 
 * ステージの階層構造における位置:
 * Stage → **Area** → WaveManager → Wave → Enemy
 * 
 * 主な機能:
 * - プレイヤーの侵入検知とエリア自動開始
 * - エリア内のウェーブシーケンス管理（WaveManagerを使用）
 * - エリアの有効/無効切り替え（複数エリアの順次進行制御）
 * - エリアクリア判定とコールバック
 * - OBBコライダーによるエリア境界の定義
 * 
 * 使用例:
 * @code
 * // エリアの生成
 * std::vector<Wave> waves = CreateWavesFromJSON();
 * Area area(objCommon, lightManager, enemyManager, waves);
 * 
 * // エリアクリア時のコールバック設定
 * area.SetOnClearCallback([]() {
 *     std::cout << "Area Cleared!" << std::endl;
 * });
 * 
 * // エリアの位置とサイズを設定
 * area.GetAreaObject()->SetPosition({100.0f, 0.0f, 0.0f});
 * area.GetAreaObject()->SetScale({50.0f, 10.0f, 50.0f});
 * 
 * // エリアを有効化（プレイヤーの侵入を検知可能にする）
 * area.SetActive(true);
 * @endcode
 */
class Area
{
public:
    /**
     * @brief コンストラクタ
     * @param objCommon 3Dオブジェクト共通データへのポインタ
     * @param lightManager ライトマネージャーへのポインタ
     * @param enemyManager 敵マネージャーへのポインタ
     * @param waves このエリアで発生するウェーブのリスト
     */
    Area(Object3dCommon* objCommon, LightManager* lightManager, EnemyManager* enemyManager, const std::vector<Wave>& waves);

    /**
     * @brief エリアを開始する
     * 
     * ウェーブシーケンスを開始します。
     * 通常はプレイヤーがエリアに侵入した際に自動的に呼ばれますが、
     * 手動で開始することも可能です。
     * 既に開始されている場合は何もしません（冪等性を保証）。
     */
    void Start();

    /**
     * @brief エリアの更新処理
     * 
     * ウェーブマネージャーの更新とエリアオブジェクトの更新を行います。
     * 毎フレーム呼び出す必要があります。
     * 
     * @param camera カメラマネージャーへのポインタ（トランスフォーム計算に使用）
     */
    void Update(CameraManager* camera);

    /**
     * @brief エリアクリア時のコールバック関数を設定
     * 
     * このコールバックは、エリア内の全ウェーブがクリアされた際に呼ばれます。
     * 次のエリアへの遷移などに使用されます。
     * 
     * @param cb クリア時に実行される関数オブジェクト
     */
    void SetOnClearCallback(std::function<void()> cb) { onClearCallback_ = std::move(cb); }

    /**
     * @brief エリアがクリアされているかを取得
     * @return true: クリア済み, false: 未クリア
     */
    bool IsCleared() const { return isCleared_; }

    /**
     * @brief エリアが開始されているかを取得
     * @return true: 開始済み, false: 未開始
     */
    bool IsStarted() const { return isStarted_; }

    /**
     * @brief エリアがアクティブかを取得
     * @return true: アクティブ（更新・判定が有効）, false: 非アクティブ
     */
	bool IsActive() const { return isActive_; }

    /**
     * @brief エリアのアクティブ状態を設定
     * 
     * 非アクティブなエリアは、プレイヤーの侵入を検知せず、更新も行われません。
     * 複数エリアを順番に進行させる際に使用します。
     * 
     * @param active true: アクティブ化, false: 非アクティブ化
     */
    void SetActive(bool active);

	/**
	 * @brief エリアの判定用ゲームオブジェクトを取得
	 * 
	 * エリアの境界を定義するOBBコライダーを持つゲームオブジェクトです。
	 * エリアの位置、回転、スケールを設定する際に使用します。
	 * 
	 * @return エリアオブジェクトへのポインタ
	 */
	GameObject* GetAreaObject() const { return areaObject_.get(); }

private:
    WaveManager waveManager_;    ///< ウェーブシーケンスを管理するマネージャー
    std::unique_ptr<GameObject> areaObject_; ///< エリアの判定用ゲームオブジェクト（OBBコライダー付き）
    bool isStarted_;             ///< エリアが開始されているか
    bool isCleared_;             ///< エリアがクリアされているか
    bool isActive_;              ///< エリアがアクティブか（更新・判定が有効）
    std::function<void()> onClearCallback_; ///< クリア時のコールバック関数
    
};