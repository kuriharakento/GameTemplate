#pragma once

#include <functional>
#include <memory>
#include "GameUI.h"
#include "graphics/2d/FontSprite.h"
#include "graphics/2d/Sprite.h"
#include "math/Vector2.h"

/**
 * @brief ポーズメニュークラス
 *
 * ESCキーでメニューを開閉し、再開・リトライ・終了を選択できる。
 * ポーズ中はTimeManagerのゲーム時間を停止する。
 */
class PoseMenu
{
public:
	PoseMenu() = default;
	~PoseMenu();

	/**
	 * @brief 初期化
	 * @param spriteCommon スプライト共通設定
	 */
	void Initialize(SpriteCommon* spriteCommon);

	/**
	 * @brief 更新処理（毎フレーム呼ぶ）
	 */
	void Update();

	/**
	 * @brief 描画処理
	 */
	void Draw();

	/**
	 * @brief ImGuiデバッグUI描画
	 */
	void DrawImGui();

	/*---------------[ ポーズ状態 ]---------------*/

	/**
	 * @brief ポーズ中かどうか
	 * @return ポーズ中ならtrue
	 */
	bool IsPaused() const { return isPaused_; }

	/**
	 * @brief ポーズ状態を設定
	 * @param paused trueでポーズ開始
	 */
	void SetPaused(bool paused);

	/*---------------[ コールバック設定 ]---------------*/

	/**
	 * @brief 再開ボタン押下時のコールバック
	 * @param callback コールバック関数
	 */
	void SetOnResumeCallback(const std::function<void()>& callback) { onResumeCallback_ = callback; }

	/**
	 * @brief リトライボタン押下時のコールバック
	 * @param callback コールバック関数
	 */
	void SetOnRetryCallback(const std::function<void()>& callback) { onRetryCallback_ = callback; }

	/**
	 * @brief タイトルボタン押下時のコールバック
	 * @param callback コールバック関数
	 */
	void SetOnTitleCallback(const std::function<void()>& callback) { onTitleCallback_ = callback; }

	/**
	 * @brief 終了ボタン押下時のコールバック
	 * @param callback コールバック関数
	 */
	void SetOnExitCallback(const std::function<void()>& callback) { onExitCallback_ = callback; }

private:
	// レイアウト更新
	void UpdateLayout();

	// ホバーアニメーション適用
	void ApplyHoverAnimation();

private:
	// ポーズ状態フラグ
	bool isPaused_ = false;

	// 背景オーバーレイ（半透明黒）
	std::unique_ptr<Sprite> backgroundOverlay_;

	// メニューボタン
	std::unique_ptr<GameUI> resumeButton_;
	std::unique_ptr<GameUI> retryButton_;
	std::unique_ptr<GameUI> toTitleButton_;
	std::unique_ptr<GameUI> exitButton_;

	// テキスト表示
	std::unique_ptr<FontSprite> titleText_;
	std::unique_ptr<FontSprite> resumeText_;
	std::unique_ptr<FontSprite> retryText_;
	std::unique_ptr<FontSprite> toTitleText_;
	std::unique_ptr<FontSprite> exitText_;

	// コールバック
	std::function<void()> onResumeCallback_;
	std::function<void()> onRetryCallback_;
	std::function<void()> onTitleCallback_;
	std::function<void()> onExitCallback_;

	// レイアウト設定
	Vector2 menuPosition_ = { 640.0f, 360.0f };  // メニュー中心位置
	Vector2 buttonSize_ = { 500.0f, 74.0f };     // ボタンサイズ
	float buttonSpacing_ = 35.0f;                 // ボタン間隔

	// タイトル設定
	Vector2 titleOffset_ = { -352.0f, -240.0f };
	float titleScale_ = 1.1f;
	Vector4 titleColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };

	// ボタンテキスト設定
	float textScale_ = 0.55f;
	Vector2 resumeTextOffset_ = { -176.0f, 5.0f };
	Vector2 retryTextOffset_ = { -141.0f, 5.0f };
	Vector2 toTitleTextOffset_ = { -141.0f, 5.0f };
	Vector2 exitTextOffset_ = { -106.0f, 5.0f };
	Vector4 textColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };

	// 背景オーバーレイ設定
	Vector4 overlayColor_ = { 0.0f, 0.0f, 0.0f, 0.7f };

	// ボタン色設定
	Vector4 buttonNormalColor_ = { 0.302f, 0.302f, 0.302f, 0.902f };   // 通常色 (77/255)
	Vector4 buttonHoverColor_ = { 0.502f, 0.502f, 0.502f, 0.902f };    // ホバー色 (128/255)

	// ホバーアニメーション設定
	float hoverSlideOffset_ = 40.0f;      // スライド量（右方向）
	float hoverScaleMultiplier_ = 1.08f;  // スケール倍率
	float hoverAnimationSpeed_ = 8.0f;    // アニメーション速度

	// 各ボタンのホバーアニメーション進行度（0.0〜1.0）
	float resumeHoverProgress_ = 0.0f;
	float retryHoverProgress_ = 0.0f;
	float titleHoverProgress_ = 0.0f;
	float exitHoverProgress_ = 0.0f;

	// 各ボタンのホバー状態
	bool isResumeHovered_ = false;
	bool isRetryHovered_ = false;
	bool isTitleHovered_ = false;
	bool isExitHovered_ = false;

	// スクリーンサイズ
	static inline const float kScreenWidth = 1280.0f;
	static inline const float kScreenHeight = 720.0f;
};
