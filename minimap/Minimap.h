#pragma once
#include "application/stage/StageManager.h"
#include "graphics/2d/Sprite.h"

/**
 * @brief ミニマップUI表示クラス
 * 
 * ゲーム画面の一部に表示されるミニマップを管理します。
 * プレイヤー位置、敵位置、エリア情報をリアルタイムでマップ上に表示します。
 * ワールド座標をミニマップのスクリーン座標に変換して描画します。
 * 
 * @note ワールド座標系（3D空間）からミニマップのローカル2D座標への変換を行います
 * @note 敵やエリアの数に応じて動的にアイコンを生成・管理します
 */
class Minimap
{
public:
    /**
     * @brief ミニマップの初期化
     * 
     * フレーム、アイコン用スプライトを初期化し、ステージ情報を設定します。
     * 
     * @param spriteCommon スプライト共通設定へのポインタ
     * @param stageManager ステージ管理クラスへのポインタ（敵・エリア情報の取得に使用）
     */
    void Initialize(SpriteCommon* spriteCommon, StageManager* stageManager);
    ~Minimap();
    
    /**
     * @brief 毎フレームの更新処理
     * 
     * プレイヤー、敵、エリアの位置を取得し、ミニマップ上のアイコン位置を更新します。
     */
    void Update();
    void DrawImGui();
    
    /**
     * @brief ミニマップの描画
     * 
     * フレーム、プレイヤーアイコン、敵アイコン、エリアアイコンを描画します。
     */
    void Draw();

private:
    /**
     * @brief ワールド座標をミニマップ座標に変換
     * 
     * 3Dワールド空間の座標をミニマップのスクリーン座標に変換します。
     * マップの中心を基準として、X-Z平面をミニマップの2D平面にマッピングします。
     * 
     * @param worldPos 変換元のワールド座標（3D）
     * @return Vector2 変換後のミニマップスクリーン座標（2D）
     */
    Vector2 WorldToMinimap(const Vector3& worldPos)const;

private:
    // スプライト共通設定（非所有ポインタ）
    SpriteCommon* spriteCommon_ = nullptr;
    // ステージ管理クラス（非所有ポインタ）
    StageManager* stageManager_ = nullptr;

    // ミニマップのフレーム（枠）
    std::unique_ptr<Sprite> frame_;
    // 敵アイコン（敵の数だけ動的生成）
    std::vector<std::unique_ptr<Sprite>> enemyIcons_;
    // プレイヤーアイコン
    std::unique_ptr< Sprite> playerIcon_;
    // エリアアイコン（エリアの数だけ動的生成）
    std::vector<std::unique_ptr<Sprite>> areaIcon_;
    // 各エリアのアクティブ状態フラグ
    std::vector<bool> areaActiveFlags_;
    // ミニマップが表すワールド空間の幅
    float mapWidth_ = 200.0f;
    // ミニマップが表すワールド空間の高さ
    float mapHeight_ = 200.0f;
};

