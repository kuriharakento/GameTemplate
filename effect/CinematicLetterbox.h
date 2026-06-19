#pragma once
#include <memory>
#include "graphics/2d/Sprite.h"
#include "math/Easing.h"

/**
 * @brief レターボックスアニメーションのイージングタイプ
 * 
 * シネマティックな演出で使用するレターボックスの表示/非表示アニメーションに適用するイージング関数を定義します。
 * 各イージングタイプは異なる動きの特性を持ち、演出に応じて使い分けることができます。
 */
enum class LetterboxEase
{
    Linear,         ///< 線形補間（等速）
    InSine,         ///< サイン曲線による加速
    OutSine,        ///< サイン曲線による減速
    InOutSine,      ///< サイン曲線による加速・減速
    InQuint,        ///< 5乗による加速
    OutQuint,       ///< 5乗による減速
    InOutQuint,     ///< 5乗による加速・減速
    InCirc,         ///< 円形曲線による加速
    OutCirc,        ///< 円形曲線による減速
    InOutCirc,      ///< 円形曲線による加速・減速
    InElastic,      ///< 弾性的な動き（開始時にオーバーシュート）
    OutElastic,     ///< 弾性的な動き（終了時にオーバーシュート）
    InOutElastic,   ///< 弾性的な動き（両端でオーバーシュート）
    InExpo,         ///< 指数関数による加速
    OutExpo,        ///< 指数関数による減速
    InOutExpo,      ///< 指数関数による加速・減速
    OutQuad,        ///< 2乗による減速
    InOutQuart,     ///< 4乗による加速・減速
    InBack,         ///< 後ろに引いてから加速
    OutBack,        ///< 前に出てから減速
    InOutBack,      ///< 後ろに引いて前に出る加速・減速
    OutBounce,      ///< バウンドする減速
    InBounce,       ///< バウンドする加速
    InOutBounce     ///< バウンドする加速・減速
};

/**
 * @brief レターボックスの表示状態
 * 
 * レターボックスエフェクトの現在の状態を管理します。
 * 状態遷移により、適切なアニメーション処理が実行されます。
 */
enum class LetterboxState
{
    Hidden,    ///< 完全に隠れている状態（レターボックスが非表示）
    Showing,   ///< 表示アニメーション中
    Visible,   ///< 完全に表示されている状態（レターボックスが画面に表示）
    Hiding     ///< 非表示アニメーション中
};

/**
 * @brief シネマティックレターボックスエフェクトクラス
 * 
 * 映画のような演出を実現するための、画面上下に表示される黒帯（レターボックス）エフェクトを管理します。
 * イージング機能により、スムーズなアニメーションで表示・非表示を制御できます。
 * 
 * 主な機能:
 * - 多様なイージングタイプによる様々なアニメーション
 * - オーバーシュートに対応した描画範囲管理
 * - カスタマイズ可能な色と高さ設定
 * - ImGuiによるリアルタイムパラメータ調整
 * 
 * @code
 * // 使用例
 * CinematicLetterbox letterbox;
 * letterbox.Initialize(spriteCommon, texturePath, 1280.0f, 720.0f);
 * letterbox.SetEaseType(LetterboxEase::InOutBack);
 * letterbox.Show(1.0f);  // 1秒かけて表示
 * @endcode
 */
class CinematicLetterbox
{
public:
    CinematicLetterbox();
    ~CinematicLetterbox();

    /**
     * @brief レターボックスの初期化
     * 
     * スプライトを作成し画面サイズに応じた初期設定を行います。
     * 上下に配置される2つのバーは、オーバーシュート用のマージンを含めて作成されます。
     * 
     * @param spriteCommon スプライト共通設定へのポインタ
     * @param texturePath レターボックスに使用するテクスチャのパス
     * @param screenWidth 画面幅（ピクセル）
     * @param screenHeight 画面高さ（ピクセル）
     */
    void Initialize(SpriteCommon* spriteCommon, const std::string& texturePath, float screenWidth, float screenHeight);

    /**
     * @brief レターボックスを表示
     * 
     * 非表示状態または非表示中の場合にのみ、表示アニメーションを開始します。
     * 既に表示中または表示済みの場合は何もしません。
     * 
     * @param duration アニメーション時間（秒）、デフォルトは1.0秒
     */
    void Show(float duration = 1.0f);

    /**
     * @brief レターボックスを非表示
     * 
     * 表示状態または表示中の場合にのみ、非表示アニメーションを開始します。
     * 既に非表示中または非表示済みの場合は何もしません。
     * 
     * @param duration アニメーション時間（秒）、デフォルトは1.0秒
     */
    void Hide(float duration = 1.0f);

    /**
     * @brief レターボックスの更新処理
     * 
     * 毎フレーム呼び出され、アニメーション進行状態を更新します。
     * 状態に応じてイージング計算を行い、バーの位置を更新します。
     */
    void Update();

    /**
     * @brief レターボックスの描画
     * 
     * 現在の状態に応じてレターボックスを描画します。
     * 完全に非表示の場合は描画処理をスキップします。
     */
    void Draw();

    /**
     * @brief 現在の状態を取得
     * @return 現在のレターボックスの状態
     */
    LetterboxState GetState() const { return state_; }

    /**
     * @brief イージングタイプを設定
     * @param type 適用するイージングタイプ
     */
    void SetEaseType(LetterboxEase type) { easeType_ = type; }
    
    /**
     * @brief レターボックスの高さを設定
     * 
     * 画面上下それぞれのレターボックスの高さを変更します。
     * サイズと位置が同時に更新されます。
     * 
     * @param height レターボックスの高さ（ピクセル）
     */
    void SetLetterboxHeight(float height);
    
    /**
     * @brief レターボックスの色を設定
     * @param color 設定する色（RGBA）
     */
    void SetColor(const Vector4& color);

    /**
     * @brief ImGuiデバッグウィンドウの表示
     * 
     * 状態の可視化、パラメータ調整などをImGuiで表示します。
     * USE_IMGUIマクロが定義されている場合のみ有効です。
     */
    void ShowImGui();

private:
    /**
     * @brief イージング関数を適用
     * 
     * 設定されたイージングタイプに基づいて、補間値を計算します。
     * 
     * @param t 正規化された時間（0.0〜1.0）
     * @return イージングを適用した値（0.0〜1.0、オーバーシュート系は範囲外も可）
     */
    float ApplyEasing(float t) const;
    
    /**
     * @brief バーの位置を更新
     * 
     * 現在の進行度に基づいて、上下のバーのY座標を計算して更新します。
     */
    void UpdateBarPositions();
    
    /**
     * @brief バーのサイズを更新
     * 
     * レターボックスの高さとオーバーシュートマージンを含めてサイズを更新します。
     */
    void UpdateBarSizes();

private:
    float screenWidth_ = 0.0f;    ///< 画面幅（Initializeで設定）
    float screenHeight_ = 0.0f;   ///< 画面高さ（Initializeで設定）

    float letterboxHeight_ = 100.0f; ///< レターボックスの高さ（画面の上下それぞれ）

    float overshootMargin_ = 100.0f; ///< オーバーシュート用の余白（BackやElasticイージングで範囲外に出る用）

    float duration_ = 1.0f;  ///< アニメーション時間（秒）
    float elapsed_ = 0.0f;   ///< 経過時間（秒）
    float progress_ = 0.0f;  ///< アニメーション進行度（0.0=完全に隠れている, 1.0=完全に表示）

    LetterboxState state_ = LetterboxState::Hidden;            ///< 現在の状態
    LetterboxEase easeType_ = LetterboxEase::InOutBack;        ///< 適用するイージングタイプ

    Vector4 color_ = { 0.0f, 0.0f, 0.0f, 1.0f }; ///< バーの色（デフォルトは黒）

    std::unique_ptr<Sprite> topBar_;    ///< 上部のレターボックスバー
    std::unique_ptr<Sprite> bottomBar_; ///< 下部のレターボックスバー
};
