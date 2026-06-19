#include "ResultScene.h"

// scene
#include "engine/scene/manager/SceneManager.h"
#include "manager/effect/PostProcessManager.h"
#include "application/game/ScoreManager.h"
#include "engine/scene/factory/SceneNames.h"

// math
#include "math/VectorColorCodes.h"
#include <input/Input.h>
#include <audio/Audio.h>

#include "engine/base/WinApp.h"

void ResultScene::Initialize()
{
	transitionEffect_.Initialize(
		sceneManager_->GetSpriteCommon(),
		"./Resources/black.png",
		kTransitionGridX, kTransitionGridY,
		WinApp::kClientWidth, WinApp::kClientHeight
	);

	// ブルームを無効化
	sceneManager_->GetPostProcessManager()->bloomEffect_->SetEnabled(false);

	// カメラの初期設定
	sceneManager_->GetCameraManager()->GetActiveCamera()->SetTranslate(Vector3());
	sceneManager_->GetCameraManager()->GetActiveCamera()->SetRotate(kInitialCameraDirection);

	SpriteCommon* sprCommon = sceneManager_->GetSpriteCommon();

	// タイトルへ戻るUIの初期化
	toTitleUI_ = std::make_unique<GameUI>();
	toTitleUI_->Initialize(sprCommon, "./Resources/black.png");
	toTitleUI_->SetScreenPosition(kToTitleUIPosition);
	toTitleUI_->SetSize(kUISize);
	toTitleUI_->SetAnchorPoint(kUIAnchorPoint);
	toTitleUI_->SetColor(VectorColorCodes::Black);
	
	// コールバックの設定
	toTitleUI_->SetInteractable(true);
	toTitleUI_->SetOnClickCallback([this]() {
		returnToTitle_ = true;
		Audio::GetInstance()->PlayWave("start_se", false);
		ChangeState(SceneState::Exit);
		transitionEffect_.SetEaseType(SceneTransitionEase::InSine);
		transitionEffect_.SetFadeType(FadeType::FadeIn);
		transitionEffect_.SetMode(TransitionMode::EdgesToCenter);
		transitionEffect_.Start(kTransitionDuration, VectorColorCodes::Black, VectorColorCodes::Green);
		toTitleUI_->SetInteractable(false);
	});
	toTitleUI_->SetOnHoverStayCallback([this]() {
		toTitleUI_->SetColor(VectorColorCodes::White);
		titleFontSprite_->SetColor(VectorColorCodes::Black);
	});
	toTitleUI_->SetOnHoverExitCallback([this]() {
		toTitleUI_->SetColor(VectorColorCodes::Black);
		titleFontSprite_->SetColor(VectorColorCodes::White);
	});

	// リトライUIの初期化
	retryUI_ = std::make_unique<GameUI>();
	retryUI_->Initialize(sprCommon, "./Resources/black.png");
	retryUI_->SetScreenPosition(kRetryUIPosition);
	retryUI_->SetSize(kUISize);
	retryUI_->SetAnchorPoint(kUIAnchorPoint);
	retryUI_->SetColor(VectorColorCodes::Black);
	
	// コールバックの設定
	retryUI_->SetInteractable(true);
	retryUI_->SetOnClickCallback([this]() {
		retry_ = true;
		Audio::GetInstance()->PlayWave("start_se", false);
		ChangeState(SceneState::Exit);
		transitionEffect_.SetEaseType(SceneTransitionEase::InSine);
		transitionEffect_.SetFadeType(FadeType::FadeIn);
		transitionEffect_.SetMode(TransitionMode::EdgesToCenter);
		transitionEffect_.Start(kTransitionDuration, VectorColorCodes::Black, VectorColorCodes::Green);
		retryUI_->SetInteractable(false);
	});
	retryUI_->SetOnHoverStayCallback([this]() {
		retryUI_->SetColor(VectorColorCodes::White);
		retryFontSprite_->SetColor(VectorColorCodes::Black);
	});
	retryUI_->SetOnHoverExitCallback([this]() {
		retryUI_->SetColor(VectorColorCodes::Black);
		retryFontSprite_->SetColor(VectorColorCodes::White);
	});

	// テキスト描画の初期化
	titleFontSprite_ = std::make_unique<FontSprite>();
	titleFontSprite_->Initialize(sprCommon, "nico");
	titleFontSprite_->SetText("Title");
	titleFontSprite_->SetAlignment(FontAlignment::Center);
	titleFontSprite_->SetSpacing(kDefaultSpacing);
	titleFontSprite_->SetPosition(kTitleFontSpritePosition);
	titleFontSprite_->SetScale(kButtonFontScale);

	retryFontSprite_ = std::make_unique<FontSprite>();
	retryFontSprite_->Initialize(sprCommon, "nico");
	retryFontSprite_->SetText("Retry");
	retryFontSprite_->SetAlignment(FontAlignment::Center);
	retryFontSprite_->SetSpacing(kDefaultSpacing);
	retryFontSprite_->SetPosition(kRetryFontSpritePosition);
	retryFontSprite_->SetScale(kButtonFontScale);

	resultLogoFontSprite_ = std::make_unique<FontSprite>();
	resultLogoFontSprite_->Initialize(sprCommon, "nico");
	resultLogoFontSprite_->SetText("RESULT"); // 統一した名称に
	resultLogoFontSprite_->SetAlignment(FontAlignment::Center);
	resultLogoFontSprite_->SetSpacing(kLogoSpacing);
	resultLogoFontSprite_->SetPosition(kResultLogoPosition);
	resultLogoFontSprite_->SetScale(kLogoFontScale);

	// スコア表示
	scoreLabelFontSprite_ = std::make_unique<FontSprite>();
	scoreLabelFontSprite_->Initialize(sprCommon, "nico");
	scoreLabelFontSprite_->SetText("SCORE :");
	scoreLabelFontSprite_->SetAlignment(FontAlignment::Center);
	scoreLabelFontSprite_->SetSpacing(kDefaultSpacing);
	scoreLabelFontSprite_->SetPosition(kScoreLabelPosition);
	scoreLabelFontSprite_->SetScale(kScoreLabelScale);

	scoreNumberSprite_ = std::make_unique<NumberSprite>();
	scoreNumberSprite_->Initialize(sprCommon, "./Resources/numbers.png", { 64.0f, 64.0f }); // 既存のタイルサイズ
	scoreNumberSprite_->SetPosition(kScoreNumberPosition);
	scoreNumberSprite_->SetAlignment(NumberAlignment::Center);
	scoreNumberSprite_->SetScale(kScoreNumberScale);
	scoreNumberSprite_->SetNumber(static_cast<int>(ScoreManager::GetScore()));

	Audio::GetInstance()->LoadWave("gameclear", "bgm/gameclear.wav", SoundGroup::BGM);
	Audio::GetInstance()->PlayWave("gameclear", true);

	StartState(SceneState::Enter);
}

void ResultScene::Finalize()
{
	Audio::GetInstance()->StopWave("gameclear");
	ClearObjects();
	sceneManager_->GetPostProcessManager()->bloomEffect_->SetEnabled(true);
}

void ResultScene::Draw3D()
{
	BaseScene::Draw3D();
}

void ResultScene::DrawShadow()
{
	BaseScene::DrawShadow();
}
void ResultScene::DrawGBuffer() {}

void ResultScene::Draw2D()
{
	toTitleUI_->Draw();
	retryUI_->Draw();

	titleFontSprite_->Draw();
	retryFontSprite_->Draw();
	resultLogoFontSprite_->Draw();
	
	scoreLabelFontSprite_->Draw();
	scoreNumberSprite_->Draw();

	transitionEffect_.Draw();
}

void ResultScene::DrawImGui() {}

void ResultScene::OnEnterEnter()
{
	transitionEffect_.SetFadeType(FadeType::FadeOut);
	transitionEffect_.SetEaseType(SceneTransitionEase::InSine);
	transitionEffect_.SetMode(TransitionMode::CenterToEdges);
	transitionEffect_.Start(kTransitionDuration, VectorColorCodes::Green, VectorColorCodes::Black);
}

void ResultScene::OnUpdateEnter()
{
	transitionEffect_.Update();
	if (transitionEffect_.GetState() == TransitionState::Done)
	{
		ChangeState(SceneState::Playing);
	}
}

void ResultScene::OnExitEnter() {}
void ResultScene::OnEnterPlaying() {}
void ResultScene::OnUpdatePlaying() {}
void ResultScene::OnExitPlaying() {}

void ResultScene::OnEnterExit()
{
	transitionEffect_.SetFadeType(FadeType::FadeIn);
	transitionEffect_.SetEaseType(SceneTransitionEase::InSine);
	transitionEffect_.SetMode(TransitionMode::EdgesToCenter);
	transitionEffect_.Start(kTransitionDuration, VectorColorCodes::Green, VectorColorCodes::Black);
}

void ResultScene::OnUpdateExit()
{
	transitionEffect_.Update();
	if (transitionEffect_.GetState() == TransitionState::Done)
	{
		if (returnToTitle_)
		{
			sceneManager_->ChangeScene(SceneNames::Title);
		}
		else if (retry_)
		{
			sceneManager_->ChangeScene(SceneNames::GamePlay);
		}
	}
}

void ResultScene::OnExitExit() {}

void ResultScene::CommonUpdate()
{
	toTitleUI_->Update();
	retryUI_->Update();
	titleFontSprite_->Update();
	retryFontSprite_->Update();
	resultLogoFontSprite_->Update();
	scoreLabelFontSprite_->Update();
	scoreNumberSprite_->Update();
}
