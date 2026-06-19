#include "PoseMenu.h"
#include "input/Input.h"
#include "time/TimeManager.h"
#include "externals/imgui/imgui.h"
#ifdef USE_IMGUI
#include "manager/editor/DebugUIManager.h"
#endif
#include "math/Easing.h"
#include "math/MathUtils.h"
#include <audio/Audio.h>

void PoseMenu::Initialize(SpriteCommon* spriteCommon)
{
	// 背景オーバーレイ（半透明黒）
	backgroundOverlay_ = std::make_unique<Sprite>();
	backgroundOverlay_->Initialize(spriteCommon, "./Resources/black.png");
	backgroundOverlay_->SetPosition(Vector2(kScreenWidth / 2.0f, kScreenHeight / 2.0f));
	backgroundOverlay_->SetSize(Vector2(kScreenWidth, kScreenHeight));
	backgroundOverlay_->SetAnchorPoint(Vector2(0.5f, 0.5f));
	backgroundOverlay_->SetColor(Vector4(1.0f, 1.0f, 1.0f, 0.7f));  // 白でテクスチャ色を維持、アルファ0.7

	// 再開ボタン
	resumeButton_ = std::make_unique<GameUI>();
	resumeButton_->Initialize(spriteCommon, "./Resources/white1x1.png");
	resumeButton_->SetColor(Vector4(0.3f, 0.3f, 0.3f, 0.9f));
	resumeButton_->SetVisible(false);
	resumeButton_->SetOnClickCallback([this]() {
		Audio::GetInstance()->PlayWave("start_se", false);
		SetPaused(false);
		if (onResumeCallback_)
		{
			onResumeCallback_();
		}
									  });
	resumeButton_->SetOnHoverEnterCallback([this]() {
		isResumeHovered_ = true;
										   });
	resumeButton_->SetOnHoverExitCallback([this]() {
		isResumeHovered_ = false;
										  });

	// リトライボタン
	retryButton_ = std::make_unique<GameUI>();
	retryButton_->Initialize(spriteCommon, "./Resources/white1x1.png");
	retryButton_->SetColor(Vector4(0.3f, 0.3f, 0.3f, 0.9f));
	retryButton_->SetVisible(false);
	retryButton_->SetOnClickCallback([this]() {
		Audio::GetInstance()->PlayWave("start_se", false);
		SetPaused(false);
		if (onRetryCallback_)
		{
			onRetryCallback_();
		}
									 });
	retryButton_->SetOnHoverEnterCallback([this]() {
		isRetryHovered_ = true;
										  });
	retryButton_->SetOnHoverExitCallback([this]() {
		isRetryHovered_ = false;
										 });

	// タイトルへボタン
	toTitleButton_ = std::make_unique<GameUI>();
	toTitleButton_->Initialize(spriteCommon, "./Resources/white1x1.png");
	toTitleButton_->SetColor(Vector4(0.3f, 0.3f, 0.3f, 0.9f));
	toTitleButton_->SetVisible(false);
	toTitleButton_->SetOnClickCallback([this]() {
		Audio::GetInstance()->PlayWave("start_se", false);
		SetPaused(false);
		if (onTitleCallback_)
		{
			onTitleCallback_();
		}
									 });
	toTitleButton_->SetOnHoverEnterCallback([this]() {
		isTitleHovered_ = true;
										  });
	toTitleButton_->SetOnHoverExitCallback([this]() {
		isTitleHovered_ = false;
										 });

	// 終了ボタン
	exitButton_ = std::make_unique<GameUI>();
	exitButton_->Initialize(spriteCommon, "./Resources/white1x1.png");
	exitButton_->SetColor(Vector4(0.3f, 0.3f, 0.3f, 0.9f));
	exitButton_->SetVisible(false);
	exitButton_->SetOnClickCallback([this]() {
		Audio::GetInstance()->PlayWave("start_se", false);
		if (onExitCallback_)
		{
			onExitCallback_();
		}
									});
	exitButton_->SetOnHoverEnterCallback([this]() {
		isExitHovered_ = true;
										 });
	exitButton_->SetOnHoverExitCallback([this]() {
		isExitHovered_ = false;
										});

	// テキスト初期化
	titleText_ = std::make_unique<FontSprite>();
	titleText_->Initialize(spriteCommon, "nico");
	titleText_->SetText("PAUSED");
	titleText_->SetVisible(false);

	resumeText_ = std::make_unique<FontSprite>();
	resumeText_->Initialize(spriteCommon, "nico");
	resumeText_->SetText("Resume");
	resumeText_->SetVisible(false);

	retryText_ = std::make_unique<FontSprite>();
	retryText_->Initialize(spriteCommon, "nico");
	retryText_->SetText("Retry");
	retryText_->SetVisible(false);

	toTitleText_ = std::make_unique<FontSprite>();
	toTitleText_->Initialize(spriteCommon, "nico");
	toTitleText_->SetText("Title");
	toTitleText_->SetVisible(false);

	exitText_ = std::make_unique<FontSprite>();
	exitText_->Initialize(spriteCommon, "nico");
	exitText_->SetText("Exit");
	exitText_->SetVisible(false);

	// 初期レイアウト適用
	UpdateLayout();

#ifdef USE_IMGUI
	DebugUIManager::GetInstance()->RegisterDebugUI(this, "Pose Menu", [this]() { this->DrawImGui(); }, DebugUIArea::Inspector);
#endif
}

PoseMenu::~PoseMenu()
{
#ifdef USE_IMGUI
	if (DebugUIManager::HasInstance()) {
		DebugUIManager::GetInstance()->UnregisterDebugUI(this);
	}
#endif
}

void PoseMenu::Update()
{
	// ESCキーでポーズ切り替え
	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE))
	{
		SetPaused(!isPaused_);
	}

	// ポーズ中のみ更新
	if (isPaused_)
	{
		// 背景の更新
		backgroundOverlay_->Update();

		// ボタン更新
		resumeButton_->Update();
		retryButton_->Update();
		toTitleButton_->Update();
		exitButton_->Update();

		// ホバーアニメーション進行度を更新（UIコンテキスト使用=ポーズ中でも動く）
		const float dt = TimeManager::GetInstance().GetUIContext().realDeltaTime;
		const float animSpeed = hoverAnimationSpeed_ * dt;

		// Resume
		if (isResumeHovered_)
		{
			resumeHoverProgress_ = (std::min)(resumeHoverProgress_ + animSpeed, 1.0f);
		}
		else
		{
			resumeHoverProgress_ = (std::max)(resumeHoverProgress_ - animSpeed, 0.0f);
		}

		// Retry
		if (isRetryHovered_)
		{
			retryHoverProgress_ = (std::min)(retryHoverProgress_ + animSpeed, 1.0f);
		}
		else
		{
			retryHoverProgress_ = (std::max)(retryHoverProgress_ - animSpeed, 0.0f);
		}

		// Title
		if (isTitleHovered_)
		{
			titleHoverProgress_ = (std::min)(titleHoverProgress_ + animSpeed, 1.0f);
		}
		else
		{
			titleHoverProgress_ = (std::max)(titleHoverProgress_ - animSpeed, 0.0f);
		}

		// Exit
		if (isExitHovered_)
		{
			exitHoverProgress_ = (std::min)(exitHoverProgress_ + animSpeed, 1.0f);
		}
		else
		{
			exitHoverProgress_ = (std::max)(exitHoverProgress_ - animSpeed, 0.0f);
		}

		// アニメーション適用
		ApplyHoverAnimation();
	}
}

void PoseMenu::Draw()
{
	if (!isPaused_)
	{
		return;
	}

	// 背景オーバーレイ
	backgroundOverlay_->Draw();

	// ボタン描画
	resumeButton_->Draw();
	retryButton_->Draw();
	toTitleButton_->Draw();
	exitButton_->Draw();

	// テキスト描画
	titleText_->Draw();
	resumeText_->Draw();
	retryText_->Draw();
	toTitleText_->Draw();
	exitText_->Draw();
}

void PoseMenu::DrawImGui()
{
#ifdef USE_IMGUI
	ImGui::Text("Paused: %s", isPaused_ ? "Yes" : "No");
	ImGui::Separator();

	bool layoutChanged = false;
	bool colorChanged = false;

	if (ImGui::CollapsingHeader("Layout", ImGuiTreeNodeFlags_DefaultOpen))
	{
		layoutChanged |= ImGui::DragFloat2("Menu Position", &menuPosition_.x, 1.0f, 0.0f, kScreenWidth);
		layoutChanged |= ImGui::DragFloat2("Button Size", &buttonSize_.x, 1.0f, 10.0f, 500.0f);
		layoutChanged |= ImGui::DragFloat("Button Spacing", &buttonSpacing_, 1.0f, 0.0f, 100.0f);
	}

	if (ImGui::CollapsingHeader("Background"))
	{
		colorChanged |= ImGui::ColorEdit4("Overlay Color", &overlayColor_.x);
	}

	if (ImGui::CollapsingHeader("Button Colors"))
	{
		colorChanged |= ImGui::ColorEdit4("Normal Color", &buttonNormalColor_.x);
		colorChanged |= ImGui::ColorEdit4("Hover Color", &buttonHoverColor_.x);
	}

	if (ImGui::CollapsingHeader("Title"))
	{
		layoutChanged |= ImGui::DragFloat2("Title Offset", &titleOffset_.x, 1.0f, -300.0f, 300.0f);
		layoutChanged |= ImGui::DragFloat("Title Scale", &titleScale_, 0.1f, 0.5f, 3.0f);
		colorChanged |= ImGui::ColorEdit4("Title Color", &titleColor_.x);
	}

	if (ImGui::CollapsingHeader("Button Text"))
	{
		layoutChanged |= ImGui::DragFloat("Text Scale", &textScale_, 0.1f, 0.5f, 3.0f);
		colorChanged |= ImGui::ColorEdit4("Text Color", &textColor_.x);
		ImGui::Separator();
		layoutChanged |= ImGui::DragFloat2("Resume Offset", &resumeTextOffset_.x, 1.0f, -200.0f, 200.0f);
		layoutChanged |= ImGui::DragFloat2("Retry Offset", &retryTextOffset_.x, 1.0f, -200.0f, 200.0f);
		layoutChanged |= ImGui::DragFloat2("Title Offset", &toTitleTextOffset_.x, 1.0f, -200.0f, 200.0f);
		layoutChanged |= ImGui::DragFloat2("Exit Offset", &exitTextOffset_.x, 1.0f, -200.0f, 200.0f);
	}

	if (layoutChanged || colorChanged)
	{
		UpdateLayout();
	}
#endif
}

void PoseMenu::UpdateLayout()
{
	const float centerX = menuPosition_.x;
	const float startY = menuPosition_.y - buttonSize_.y;

	// 背景オーバーレイ
	backgroundOverlay_->SetColor(overlayColor_);

	// ボタン位置・サイズ・色更新
	resumeButton_->SetScreenPosition(Vector2(centerX, startY));
	resumeButton_->SetSize(buttonSize_);
	resumeButton_->SetColor(buttonNormalColor_);

	retryButton_->SetScreenPosition(Vector2(centerX, startY + buttonSize_.y + buttonSpacing_));
	retryButton_->SetSize(buttonSize_);
	retryButton_->SetColor(buttonNormalColor_);

	toTitleButton_->SetScreenPosition(Vector2(centerX, startY + (buttonSize_.y + buttonSpacing_) * 2.0f));
	toTitleButton_->SetSize(buttonSize_);
	toTitleButton_->SetColor(buttonNormalColor_);

	exitButton_->SetScreenPosition(Vector2(centerX, startY + (buttonSize_.y + buttonSpacing_) * 3.0f));
	exitButton_->SetSize(buttonSize_);
	exitButton_->SetColor(buttonNormalColor_);

	// タイトルテキスト
	titleText_->SetPosition(Vector2(centerX + titleOffset_.x, startY + titleOffset_.y));
	titleText_->SetScale(titleScale_);
	titleText_->SetColor(titleColor_);

	// ボタンテキスト
	resumeText_->SetPosition(Vector2(centerX + resumeTextOffset_.x, startY + resumeTextOffset_.y));
	resumeText_->SetScale(textScale_);
	resumeText_->SetColor(textColor_);

	retryText_->SetPosition(Vector2(centerX + retryTextOffset_.x, startY + buttonSize_.y + buttonSpacing_ + retryTextOffset_.y));
	retryText_->SetScale(textScale_);
	retryText_->SetColor(textColor_);

	toTitleText_->SetPosition(Vector2(centerX + toTitleTextOffset_.x, startY + (buttonSize_.y + buttonSpacing_) * 2.0f + toTitleTextOffset_.y));
	toTitleText_->SetScale(textScale_);
	toTitleText_->SetColor(textColor_);

	exitText_->SetPosition(Vector2(centerX + exitTextOffset_.x, startY + (buttonSize_.y + buttonSpacing_) * 3.0f + exitTextOffset_.y));
	exitText_->SetScale(textScale_);
	exitText_->SetColor(textColor_);
}

void PoseMenu::SetPaused(bool paused)
{
	if (isPaused_ == paused)
	{
		return;
	}

	isPaused_ = paused;

	// TimeManagerとの連携
	if (isPaused_)
	{
		TimeManager::GetInstance().Pause();
	}
	else
	{
		TimeManager::GetInstance().Resume();
	}

	// UI表示切り替え
	resumeButton_->SetVisible(isPaused_);
	retryButton_->SetVisible(isPaused_);
	toTitleButton_->SetVisible(isPaused_);
	exitButton_->SetVisible(isPaused_);
	titleText_->SetVisible(isPaused_);
	resumeText_->SetVisible(isPaused_);
	retryText_->SetVisible(isPaused_);
	toTitleText_->SetVisible(isPaused_);
	exitText_->SetVisible(isPaused_);
}

void PoseMenu::ApplyHoverAnimation()
{
	const float centerX = menuPosition_.x;
	const float startY = menuPosition_.y - buttonSize_.y;

	// イージング関数を適用
	const float resumeEased = EaseOutQuad(resumeHoverProgress_);
	const float retryEased = EaseOutQuad(retryHoverProgress_);
	const float titleEased = EaseOutQuad(titleHoverProgress_);
	const float exitEased = EaseOutQuad(exitHoverProgress_);

	// Resume ボタン
	{
		const float slideX = hoverSlideOffset_ * resumeEased;
		const float scale = MathUtils::Lerp(1.0f, hoverScaleMultiplier_, resumeEased);
		const Vector4 color = Vector4::Lerp(buttonNormalColor_, buttonHoverColor_, resumeEased);

		resumeButton_->SetScreenPosition(Vector2(centerX + slideX, startY));
		resumeButton_->SetSize(Vector2(buttonSize_.x * scale, buttonSize_.y * scale));
		resumeButton_->SetColor(color);

		// テキストも連動（buttonPosにはすでにslideXが含まれている）
		const Vector2 buttonPos = resumeButton_->GetScreenPosition();
		resumeText_->SetPosition(Vector2(buttonPos.x + resumeTextOffset_.x, buttonPos.y + resumeTextOffset_.y));
		resumeText_->SetScale(textScale_ * scale);
	}

	// Retry ボタン
	{
		const float slideX = hoverSlideOffset_ * retryEased;
		const float scale = MathUtils::Lerp(1.0f, hoverScaleMultiplier_, retryEased);
		const Vector4 color = Vector4::Lerp(buttonNormalColor_, buttonHoverColor_, retryEased);

		const float buttonY = startY + buttonSize_.y + buttonSpacing_;
		retryButton_->SetScreenPosition(Vector2(centerX + slideX, buttonY));
		retryButton_->SetSize(Vector2(buttonSize_.x * scale, buttonSize_.y * scale));
		retryButton_->SetColor(color);

		// テキストも連動（buttonPosにはすでにslideXが含まれている）
		const Vector2 buttonPos = retryButton_->GetScreenPosition();
		retryText_->SetPosition(Vector2(buttonPos.x + retryTextOffset_.x, buttonPos.y + retryTextOffset_.y));
		retryText_->SetScale(textScale_ * scale);
	}

	// Title ボタン
	{
		const float slideX = hoverSlideOffset_ * titleEased;
		const float scale = MathUtils::Lerp(1.0f, hoverScaleMultiplier_, titleEased);
		const Vector4 color = Vector4::Lerp(buttonNormalColor_, buttonHoverColor_, titleEased);

		const float buttonY = startY + (buttonSize_.y + buttonSpacing_) * 2.0f;
		toTitleButton_->SetScreenPosition(Vector2(centerX + slideX, buttonY));
		toTitleButton_->SetSize(Vector2(buttonSize_.x * scale, buttonSize_.y * scale));
		toTitleButton_->SetColor(color);

		// テキストも連動（buttonPosにはすでにslideXが含まれている）
		const Vector2 buttonPos = toTitleButton_->GetScreenPosition();
		toTitleText_->SetPosition(Vector2(buttonPos.x + toTitleTextOffset_.x, buttonPos.y + toTitleTextOffset_.y));
		toTitleText_->SetScale(textScale_ * scale);
	}

	// Exit ボタン
	{
		const float slideX = hoverSlideOffset_ * exitEased;
		const float scale = MathUtils::Lerp(1.0f, hoverScaleMultiplier_, exitEased);
		const Vector4 color = Vector4::Lerp(buttonNormalColor_, buttonHoverColor_, exitEased);

		const float buttonY = startY + (buttonSize_.y + buttonSpacing_) * 3.0f;
		exitButton_->SetScreenPosition(Vector2(centerX + slideX, buttonY));
		exitButton_->SetSize(Vector2(buttonSize_.x * scale, buttonSize_.y * scale));
		exitButton_->SetColor(color);

		// テキストも連動
		const Vector2 buttonPos = exitButton_->GetScreenPosition();
		exitText_->SetPosition(Vector2(buttonPos.x + exitTextOffset_.x, buttonPos.y + exitTextOffset_.y));
		exitText_->SetScale(textScale_ * scale);
	}
}
