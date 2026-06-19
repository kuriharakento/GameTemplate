#include "SkillSelectionUI.h"
#include "application/UI/GameUI.h"
#include "graphics/2d/FontSprite.h"
#include "input/Input.h"
#include "base/WinApp.h"
#include "math/MathUtils.h"
#include "engine/time/TimeManager.h"


void SkillSelectionUI::Initialize(SpriteCommon* spriteCommon)
{
	spriteCommon_ = spriteCommon;

	// 半透明黒のオーバーレイ
	overlay_ = std::make_unique<GameUI>();
	overlay_->Initialize(spriteCommon, "./Resources/black.png");
	overlay_->SetAnchorPoint({ 0.0f, 0.0f });
	overlay_->SetScreenPosition({ 0.0f, 0.0f });
	overlay_->SetSize({ 1280.0f, 720.0f });
	overlay_->SetColor({ 0.0f, 0.0f, 0.0f, 0.7f });
	overlay_->SetVisible(false);
	overlay_->SetInteractable(false);

	mainTitleFont_ = std::make_unique<FontSprite>();
	mainTitleFont_->Initialize(spriteCommon, "nico");
	mainTitleFont_->SetVisible(false);
}

void SkillSelectionUI::Update()
{
	if (!isActive_) return;

	float dt = TimeManager::GetInstance().GetGameContext().deltaTime;
	if (inputLockoutTimer_ > 0.0f) {
		inputLockoutTimer_ -= dt;
	}

	auto* input = Input::GetInstance();

	for (size_t i = 0; i < cards_.size(); ++i)
	{
		auto& card = cards_[i];
		if (card.bg) card.bg->Update();

		// ホバー演出
		float targetScale = (card.bg && card.bg->IsHovered()) ? kHoverScale : kNormalScale;
		card.scale += (targetScale - card.scale) * kScaleLerpSpeed;
		
		if (card.bg)
		{
			card.bg->SetSize({ kCardWidth * card.scale, kCardHeight * card.scale });
			// 旧UIの背景色付けを削除（透明にする）
			card.bg->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		}

		if (card.icon)
		{
			// アイコンもカードの拡大に合わせる
			card.icon->Update();
			// アイコンサイズを 300x400 に固定
			card.icon->SetSize({ 300.0f * card.scale, 400.0f * card.scale });
		}

		if (card.titleIcon)
		{
			card.titleIcon->Update();
			// タイトル画像のサイズは適宜調整（ここでは横幅 240px 程度を想定）
			card.titleIcon->SetSize({ 240.0f * card.scale, 60.0f * card.scale });
		}

		// 入力判定 (1, 2, 3キー または クリック)
		// 表示直後は誤操作防止のため入力を受け付けない
		if (inputLockoutTimer_ <= 0.0f)
		{
			bool keyTriggered = false;
			if (i == 0 && input->TriggerKey(DIK_1)) keyTriggered = true;
			if (i == 1 && input->TriggerKey(DIK_2)) keyTriggered = true;
			if (i == 2 && input->TriggerKey(DIK_3)) keyTriggered = true;

			if (keyTriggered || (card.bg && card.bg->IsClicked()))
			{
				auto cb = card.onSelect;
				HideAll();
				if (cb) cb();
				break;
			}
		}
	}
}

void SkillSelectionUI::Draw()
{
	if (!isActive_) return;

	if (overlay_) overlay_->Draw();

	float centerX = 1280.0f * 0.5f;

	// タイトル
	if (mainTitleFont_)
	{
		mainTitleFont_->SetColor({ 1.0f, 1.0f, 0.4f, 1.0f });
		mainTitleFont_->DrawText("UPGRADE SELECT", { centerX - 250.0f, kCardY - kCardHeight * 0.5f - 80.0f }, kMainTitleScale);
	}

	for (auto& card : cards_)
	{
		if (card.bg) card.bg->Draw();
		
		Vector2 pos = card.bg->GetScreenPosition();

		// アイコンがあれば描画
		if (card.icon)
		{
			// カードの中央より少し上に配置
			card.icon->SetScreenPosition({ pos.x, pos.y - 40.0f });
			card.icon->Draw();
		}

		// タイトル画像
		if (card.titleIcon)
		{
			// 完全に真ん中に配置
			float titleY = pos.y;
			card.titleIcon->SetScreenPosition({ pos.x, titleY });
			card.titleIcon->Draw();
		}
	}
}

void SkillSelectionUI::Show(uint32_t level, const std::string& optionA, const std::string& optionB,
	std::function<void(int)> onSelect, const std::string& texA, const std::string& texB)
{
    std::vector<UpgradeOption> options;
    options.push_back({ optionA, "", [onSelect](){ onSelect(0); }, texA });
    options.push_back({ optionB, "", [onSelect](){ onSelect(1); }, texB });
    
    ShowUpgrades(options);
    
    // 特別なタイトル上書き
    if (mainTitleFont_) {
        // level 2 or 3
    }
}

void SkillSelectionUI::ShowUpgrades(const std::vector<UpgradeOption>& options)
{
	isActive_ = true;
	cards_.clear();

	float centerX = 1280.0f * 0.5f;
	size_t count = options.size();
	float totalWidth = (kCardWidth * count) + (kCardGap * (count - 1));
	float startX = centerX - totalWidth * 0.5f + kCardWidth * 0.5f;

	for (size_t i = 0; i < count; ++i)
	{
		CardInstance card;
		card.title = options[i].title;
		card.desc = options[i].description;
		card.onSelect = options[i].onSelect;

		card.bg = std::make_unique<GameUI>();
		card.bg->Initialize(spriteCommon_, "./Resources/white1x1.png");
		card.bg->SetAnchorPoint({ 0.5f, 0.5f });
		card.bg->SetScreenPosition({ startX + i * (kCardWidth + kCardGap), kCardY });
		card.bg->SetSize({ kCardWidth, kCardHeight });
		card.bg->SetVisible(true);
		card.bg->SetInteractable(true);

		// アイコンの設定
		if (!options[i].texturePath.empty())
		{
			card.icon = std::make_unique<GameUI>();
			card.icon->Initialize(spriteCommon_, options[i].texturePath);
			card.icon->SetAnchorPoint({ 0.5f, 0.5f });
			card.icon->SetVisible(true);
		}

		// タイトル画像の設定
		if (!options[i].titleTexturePath.empty())
		{
			card.titleIcon = std::make_unique<GameUI>();
			card.titleIcon->Initialize(spriteCommon_, options[i].titleTexturePath);
			card.titleIcon->SetAnchorPoint({ 0.5f, 0.5f });
			card.titleIcon->SetVisible(true);
		}

		cards_.push_back(std::move(card));
	}

	if (overlay_) overlay_->SetVisible(true);
	if (mainTitleFont_) mainTitleFont_->SetVisible(true);

	// 表示直後の誤操作防止タイマーをセット
	inputLockoutTimer_ = kInputLockoutTime;
}

void SkillSelectionUI::HideAll()
{
	isActive_ = false;
	cards_.clear();
	if (overlay_) overlay_->SetVisible(false);
	if (mainTitleFont_) mainTitleFont_->SetVisible(false);
}
