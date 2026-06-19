#pragma once
#include <vector>
#include <memory>
#include <string>
#include "graphics/2d/Sprite.h"
#include "math/Easing.h"

/**
 * @brief シーン遷移の方向パターン
 * 
 * グリッド状のスプライトがどの方向から順に変化していくかを定義します。
 */
enum class TransitionMode
{
    LeftTopToRightBottom,      ///< 左上から右下へ遷移
    RightBottomToLeftTop,      ///< 右下から左上へ遷移
    RightTopToLeftBottom,      ///< 右上から左下へ遷移
    LeftBottomToRightTop,      ///< 左下から右上へ遷移
    TopToBottom,               ///< 上から下へ遷移
    BottomToTop,               ///< 下から上へ遷移
    CenterToEdges,             ///< 中央から端へ遷移
    EdgesToCenter              ///< 端から中央へ遷移
};

/**
 * @brief フェードの種類
 */
enum class FadeType
{
    FadeIn,     ///< フェードイン（透明→不透明）
    FadeOut     ///< フェードアウト（不透明→透明）
};

/**
 * @brief シーン遷移のイージングタイプ
 * 
 * グリッドの透過度変化に適用するイージング関数の種類を定義します。
 */
enum class SceneTransitionEase
{
    Linear,         ///< 線形補間
    InSine,         ///< サイン加速
    OutSine,        ///< サイン減速
    InOutSine,      ///< サイン加速減速
    InQuint,        ///< 5乗加速
    OutQuint,       ///< 5乗減速
    InOutQuint,     ///< 5乗加速減速
    InCirc,         ///< 円形加速
    OutCirc,        ///< 円形減速
    InOutCirc,      ///< 円形加速減速
    InElastic,      ///< 弾性加速
    OutElastic,     ///< 弾性減速
    InOutElastic,   ///< 弾性加速減速
    InExpo,         ///< 指数加速
    OutExpo,        ///< 指数減速
    InOutExpo,      ///< 指数加速減速
    OutQuad,        ///< 2乗減速
    InOutQuart,     ///< 4乗加速減速
    InBack,         ///< 後退加速
    OutBack,        ///< 後退減速
    InOutBack,      ///< 後退加速減速
    OutBounce,      ///< バウンド減速
    InBounce,       ///< バウンド加速
    InOutBounce     ///< バウンド加速減速
};

/**
 * @brief 遷移エフェクトの状態
 */
enum class TransitionState { 
    Idle,       ///< 待機中（再生前）
    Playing,    ///< 再生中
    Done        ///< 完了
};

/**
 * @brief シーン遷移エフェクトクラス
 * 
 * 画面全体をグリッド状に分割し、各セルが順次フェードイン/アウトすることで
 * 視覚的に楽しいシーン遷移演出を実現します。
 * 
 * 主な機能:
 * - カスタマイズ可能なグリッドサイズ
 * - 8種類の遷移方向パターン
 * - グラデーションカラー対応
 * - 多様なイージング関数
 * - フェードイン/フェードアウト切り替え
 * 
 * @code
 * // 使用例
 * SceneTransitionEffect transition;
 * transition.Initialize(spriteCommon, texPath, 6, 4, 1280.0f, 720.0f);
 * transition.SetMode(TransitionMode::CenterToEdges);
 * transition.SetFadeType(FadeType::FadeOut);
 * transition.Start(1.5f, {1,1,1,1}, {0,0,0,1});  // 白から黒へグラデーション
 * @endcode
 */
class SceneTransitionEffect
{
public:
    SceneTransitionEffect();
    ~SceneTransitionEffect();

    /**
     * @brief エフェクトの初期化
     * 
     * グリッド状のスプライトを生成し画面全体を覆うように配置します。
     * 
     * @param spriteCommon スプライト共通設定
     * @param texturePath 使用するテクスチャパス
     * @param gridX 横方向のグリッド分割数
     * @param gridY 縦方向のグリッド分割数
     * @param screenWidth 画面幅（ピクセル）
     * @param screenHeight 画面高さ（ピクセル）
     */
    void Initialize(SpriteCommon* spriteCommon, const std::string& texturePath, int gridX, int gridY, float screenWidth, float screenHeight);

    /**
     * @brief グラデーションカラー付き遷移の開始
     * 
     * 開始色から終了色へのグラデーションを適用した遷移エフェクトを開始します。
     * 
     * @param duration 遷移時間（秒）
     * @param startColor 開始時の色（RGBA）
     * @param endColor 終了時の色（RGBA）
     */
    void Start(float duration, const Vector4& startColor, const Vector4& endColor);

    /**
     * @brief エフェクトの更新
     * 
     * 毎フレーム呼び出され、グリッドの透過度を更新します。
     */
    void Update();
    
    /**
     * @brief エフェクトの描画
     * 
     * 全てのグリッドスプライトを描画します。
     */
    void Draw();

    /**
     * @brief 現在の状態を取得
     * @return 遷移エフェクトの状態
     */
    TransitionState GetState() const;
    
    /**
     * @brief 状態を設定
     * @param state 設定する状態
     */
    void SetState(TransitionState state);
    
    /**
     * @brief イージングタイプを設定
     * @param type 適用するイージングタイプ
     */
    void SetEaseType(SceneTransitionEase type);
    
    /**
     * @brief 遷移モードを設定
     * @param mode 遷移の方向パターン
     */
    void SetMode(TransitionMode mode);
    
    /**
     * @brief フェードタイプを設定
     * @param type フェードイン/フェードアウトの選択
     */
    void SetFadeType(FadeType type);

    /**
     * @brief ImGuiデバッグウィンドウの表示
     */
    void ShowImGui();

private:
    /**
     * @brief イージング関数を適用
     * @param t 正規化された時間（0.0〜1.0）
     * @return イージング適用後の値
     */
    float ApplyEasing(float t) const;
    
    /**
     * @brief 色の線形補間
     * @param c0 開始色
     * @param c1 終了色
     * @param t 補間係数（0.0〜1.0）
     * @return 補間された色
     */
    Vector4 LerpColor(const Vector4& c0, const Vector4& c1, float t) const;
    
    /**
     * @brief グリッド位置に基づく進行度を計算
     * 
     * 遷移モードに応じて、各グリッドセルの変化開始タイミングを確定します。
     * 
     * @param x グリッドのX座標
     * @param y グリッドのY座標
     * @return グリッド進行度（0.0〜1.0）
     */
    float CalcGridProgress(int x, int y) const;

    int gridX_ = 6;                                         ///< 横方向のグリッド分割数
    int gridY_ = 4;                                         ///< 縦方向のグリッド分割数
    float screenWidth_ = 0.0f;                              ///< 画面幅（Initializeで設定）
    float screenHeight_ = 0.0f;                             ///< 画面高さ（Initializeで設定）
    float transitionRate_ = 0.0f;                           ///< 全体の遷移進行度（0.0〜1.0）
    SceneTransitionEase easeType_ = SceneTransitionEase::Linear;  ///< イージングタイプ
    float duration_ = 1.0f;                                 ///< 遷移時間（秒）
    float elapsed_ = 0.0f;                                  ///< 経過時間（秒）
    TransitionState state_ = TransitionState::Idle;         ///< 現在の状態
    Vector4 startColor_ = { 1.0f,1.0f,1.0f,1.0f };          ///< 開始時の色
    Vector4 endColor_ = { 1.0f,1.0f,1.0f,1.0f };            ///< 終了時の色
    TransitionMode mode_ = TransitionMode::LeftTopToRightBottom;  ///< 遷移モード
    FadeType fadeType_ = FadeType::FadeOut;                 ///< フェードタイプ

    std::vector<std::vector<std::unique_ptr<Sprite>>> gridSprites_;  ///< グリッド状に配置されたスプライト群
};
