#pragma once
#include "scene/interface/BaseScene.h"
#include <memory>
#include <vector>

// graphics
#include "graphics/3d/Object3d.h"
#include "graphics/3d/SkinnedObject3d.h"
// manager
#include "manager/scene/LightManager.h"
#include <camerawork/debug/DebugCamera.h>
#include <application/effect/SceneTransitionEffect.h>
#include <application/ui/GameUI.h>
#include <graphics/2d/FontSprite.h>
#include <graphics/2d/NumberSprite.h>

/**
 * @brief リザルトシーン
 */
class ResultScene : public BaseScene
{
public:
	void Initialize() override;
	void Finalize() override;
	void Draw2D() override;
	void Draw3D() override;
	void DrawShadow() override;
	void DrawGBuffer() override;
	void DrawImGui() override;

protected:
	void OnEnterEnter() override;
	void OnUpdateEnter() override;
	void OnExitEnter() override;

	void OnEnterPlaying() override;
	void OnUpdatePlaying() override;
	void OnExitPlaying() override;

	void OnEnterExit() override;
	void OnUpdateExit() override;
	void OnExitExit() override;

	void CommonUpdate() override;

private:
	// カメラの初期方向
	static constexpr Vector3 kInitialCameraDirection = { 0.0f, -1.2f, 0.0f };
	// タイトルUIの位置
	static constexpr Vector2 kToTitleUIPosition = { 360.0f, 620.0f };
	// リトライUIの位置
	static constexpr Vector2 kRetryUIPosition = { 920.0f, 620.0f };
	// UIのサイズ
	static constexpr Vector2 kUISize = { 300.0f, 80.0f };
	// UIのアンカーポイント
	static constexpr Vector2 kUIAnchorPoint = { 0.5f, 0.5f };
	// タイトルフォントスプライトの位置
	static constexpr Vector2 kTitleFontSpritePosition = { 360.0f, 620.0f };
	// リトライフォントスプライトの位置
	static constexpr Vector2 kRetryFontSpritePosition = { 920.0f, 620.0f };
	
	// リザルトロゴの位置
	static constexpr Vector2 kResultLogoPosition = { 640.0f, 150.0f };
	
	// スコア表示の位置
	static constexpr Vector2 kScoreLabelPosition = { 640.0f, 350.0f };
	static constexpr Vector2 kScoreNumberPosition = { 640.0f, 430.0f };

	// フォントスケール
	static constexpr float kButtonFontScale = 0.4f;
	static constexpr float kLogoFontScale = 0.8f;
	static constexpr float kScoreLabelScale = 0.5f;
	static constexpr float kScoreNumberScale = 1.5f;

	// フォントスペーシング
	static constexpr float kLogoSpacing = -40.0f;
	static constexpr float kDefaultSpacing = -20.0f;

	// トランジション
	static constexpr int kTransitionGridX = 22;
	static constexpr int kTransitionGridY = 16;
	static constexpr float kTransitionDuration = 1.0f;

	// タイトルへ戻るフラグ
	bool returnToTitle_ = false;
	// リトライフラグ
	bool retry_ = false;

	// シーン遷移エフェクト（フェードイン/アウト）
	SceneTransitionEffect transitionEffect_;
	
	// UI
	std::unique_ptr<GameUI> toTitleUI_;
	std::unique_ptr<GameUI> retryUI_;
	
	// テキスト
	std::unique_ptr<FontSprite> resultLogoFontSprite_;
	std::unique_ptr<FontSprite> titleFontSprite_;
	std::unique_ptr<FontSprite> retryFontSprite_;
	std::unique_ptr<FontSprite> scoreLabelFontSprite_;

	// スコア数値
	std::unique_ptr<NumberSprite> scoreNumberSprite_;
};
