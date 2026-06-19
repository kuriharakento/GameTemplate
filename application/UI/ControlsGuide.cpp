#include "ControlsGuide.h"
#include "application/ecs/components/SkillComponent.h"
#include "application/ecs/components/StatusComponent.h"
#include "application/ecs/components/PlayerProgressionComponent.h"
#include "math/VectorColorCodes.h"
#include "manager/graphics/TextureManager.h"
#include <algorithm>

using namespace ecs;

void ControlsGuide::Initialize(SpriteCommon* spriteCommon, Registry* registry, EntityID playerEntity)
{
	spriteCommon_ = spriteCommon;
	registry_ = registry;
	playerEntity_ = playerEntity;

	// プロンプトをクリア
	prompts_.clear();

	// スキルの順に追加（新スキル構成: LMB, Q, E, R）
	AddPrompt("LMB", "./Resources/UI/button/LMB.png", "./Resources/UI/text/LMB.png");
	AddPrompt("Q", "./Resources/UI/button/Q.png", "./Resources/UI/text/Q.png");
	AddPrompt("E", "./Resources/UI/button/E.png", "./Resources/UI/text/E.png");
	AddPrompt("R", "./Resources/UI/button/R.png", "./Resources/UI/text/R.png");

	// HPバーの初期化
	// 中身
	hpBarFG_ = std::make_unique<Sprite>();
	hpBarFG_->Initialize(spriteCommon_, "./Resources/UI/hp_bar_fill.png");
	hpBarFG_->SetPosition({ kHpBarX, kHpBarY });
	hpBarFG_->SetSize({ kHpBarWidth, kHpBarHeight });
	hpBarFG_->SetColor(VectorColorCodes::Lime);
	hpBarFG_->SetAnchorPoint({ 0.0f, 0.5f });

	// フレーム
	hpBarFrame_ = std::make_unique<Sprite>();
	hpBarFrame_->Initialize(spriteCommon_, "./Resources/UI/hp_bar_frame.png");
	hpBarFrame_->SetPosition({ kHpBarX - 4.0f, kHpBarY });
	hpBarFrame_->SetSize({ kHpBarWidth + 8.0f, kHpBarHeight + 8.0f });
	hpBarFrame_->SetColor({ 1,1,1,1 });
	hpBarFrame_->SetAnchorPoint({ 0.0f, 0.5f });

	// EXPバーの初期化
	expBarFG_ = std::make_unique<Sprite>();
	expBarFG_->Initialize(spriteCommon_, "./Resources/white1x1.png");
	expBarFG_->SetPosition({ kExpBarX, kExpBarY });
	expBarFG_->SetSize({ kExpBarWidth, kExpBarHeight });
	expBarFG_->SetColor(VectorColorCodes::Cyan);
	expBarFG_->SetAnchorPoint({ 0.0f, 0.5f });

	expBarFrame_ = std::make_unique<Sprite>();
	expBarFrame_->Initialize(spriteCommon_, "./Resources/white1x1.png");
	expBarFrame_->SetPosition({ kExpBarX - 2.0f, kExpBarY });
	expBarFrame_->SetSize({ kExpBarWidth + 4.0f, kExpBarHeight + 4.0f });
	expBarFrame_->SetColor({ 0.2f, 0.2f, 0.2f, 1.0f });
	expBarFrame_->SetAnchorPoint({ 0.0f, 0.5f });
}

void ControlsGuide::Update()
{
	if (!isVisible_ || !registry_ || playerEntity_ == kInvalidEntity) return;

	// --- スキル情報の更新 ---
	if (registry_->HasComponent<SkillComponent>(playerEntity_))
	{
		auto& skill = registry_->GetComponent<SkillComponent>(playerEntity_);

		for (auto& prompt : prompts_)
		{
			float timer = 0.0f;
			float maxTimer = 1.0f;
			bool unlocked = false;

			if (prompt.actionName_ == "LMB") { timer = skill.lmbTimer_; maxTimer = SkillComponent::kLmbBaseCooldown * skill.lmbCooldownMultiplier_; unlocked = skill.isLmbUnlocked_; }
			else if (prompt.actionName_ == "Q") { timer = skill.baseSkillTimer_; maxTimer = SkillComponent::kBaseSkillCooldown; unlocked = (skill.route_ != SkillRoute::None); }
			else if (prompt.actionName_ == "E") { timer = skill.specialSkillTimer_; maxTimer = SkillComponent::kSpecialSkillCooldown; unlocked = (skill.special_ != SkillSpecialChoice::None); }
			else if (prompt.actionName_ == "R")
			{
				timer = SkillComponent::kBeamChargeMax - skill.beamCharge_;
				maxTimer = SkillComponent::kBeamChargeMax;
				unlocked = skill.isBeamUnlocked_;

				// 特殊処理: チャージ満タンなら timer は 0 になり、progress は 1.0 になる。
				// チャージが 0 なら timer は maxTimer になり、progress は 0.0 になる。
			}

			// クールタイム進捗 (0.0: 開始, 1.0: 完了)
			float progress = 1.0f;
			if (maxTimer > 0.0f)
			{
				progress = (std::max)(0.0f, (std::min)(1.0f, 1.0f - (timer / maxTimer)));
			}

			// テクスチャの元のサイズを取得
			const auto& metadata = TextureManager::GetInstance()->GetMetadata(prompt.iconPath_);
			float texW = static_cast<float>(metadata.width);
			float texH = static_cast<float>(metadata.height);

			// クールタイム演出 (リキッド・フィル)
			if (timer > 0.0f && unlocked)
			{
				// ベースは暗く
				prompt.icon_->SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });

				// オーバーレイ（明るい）を下から満たしていく
				float currentH = kIconSize * progress;
				prompt.overlay_->SetSize({ kIconSize, currentH });

				// テクスチャ切り出し (下から progress 分)
				float cutH = texH * progress;
				prompt.overlay_->SetTextureLeftTop({ 0.0f, texH - cutH });
				prompt.overlay_->SetTextureSize({ texW, cutH });

				// Y座標を調整（下端を固定して上に伸ばす）
				// アンカーが 0.5, 0.5 なので、全体の中心から progress 分だけ下にずらした位置が中心になる
				float yOffset = (kIconSize * 0.5f) * (1.0f - progress);
				prompt.overlay_->SetPosition({ kPromptBaseX, prompt.y_ + yOffset });
				prompt.overlay_->SetColor({ 1, 1, 1, 1 });
			}
			else
			{
				// クールタイム終了または未開放
				prompt.icon_->SetColor(unlocked ? VectorColorCodes::White : Vector4(0.2f, 0.2f, 0.2f, 1.0f));
				prompt.overlay_->SetColor({ 1, 1, 1, 0 }); // 非表示
			}

			prompt.icon_->Update();
			prompt.overlay_->Update();
			prompt.description_->Update();
		}
	}

	// --- HPバーの更新 ---
	if (registry_->HasComponent<ecs::StatusComponent>(playerEntity_))
	{
		auto& status = registry_->GetComponent<ecs::StatusComponent>(playerEntity_);
		float ratio = (std::max)(0.0f, (std::min)(1.0f, status.hp_.GetValue() / status.maxHp_.GetValue()));

		hpBarFG_->SetSize({ kHpBarWidth * ratio, kHpBarHeight });

		if (ratio < 0.25f) hpBarFG_->SetColor(VectorColorCodes::Red);
		else if (ratio < 0.5f) hpBarFG_->SetColor(VectorColorCodes::Yellow);
		else hpBarFG_->SetColor(VectorColorCodes::Lime);

		hpBarFG_->Update();
		hpBarFrame_->Update();
	}

	// --- EXPバーの更新 ---
	if (registry_->HasComponent<PlayerProgressionComponent>(playerEntity_))
	{
		auto& prog = registry_->GetComponent<PlayerProgressionComponent>(playerEntity_);
		float ratio = (std::max)(0.0f, (std::min)(1.0f, prog.currentExp_ / prog.nextLevelExp_));

		expBarFG_->SetSize({ kExpBarWidth * ratio, kExpBarHeight });
		expBarFG_->Update();
		expBarFrame_->Update();
	}
}

void ControlsGuide::Draw()
{
	if (!isVisible_) return;

	for (auto& prompt : prompts_)
	{
		prompt.icon_->Draw();
		prompt.overlay_->Draw();
		prompt.description_->Draw();
	}

	hpBarFG_->Draw();
	hpBarFrame_->Draw();

	expBarFrame_->Draw();
	expBarFG_->Draw();
}

void ControlsGuide::SetVisible(bool isVisible)
{
	isVisible_ = isVisible;
}

bool ControlsGuide::IsVisible() const
{
	return isVisible_;
}

void ControlsGuide::AddPrompt(const std::string& actionName, const std::string& iconPath, const std::string& descriptionPath)
{
	ControlPrompt prompt;
	prompt.actionName_ = actionName;
	prompt.iconPath_ = iconPath;

	float y = kPromptBaseY + (prompts_.size() * kPromptSpacingY);
	prompt.y_ = y;

	// アイコン (ベース)
	prompt.icon_ = std::make_unique<Sprite>();
	prompt.icon_->Initialize(spriteCommon_, iconPath);
	prompt.icon_->SetPosition({ kPromptBaseX, y });
	prompt.icon_->SetSize({ kIconSize, kIconSize });
	prompt.icon_->SetAnchorPoint({ 0.5f, 0.5f });
	prompt.icon_->SetColor({ 0.3f, 0.3f, 0.3f, 1.0f }); // デフォルトを少し暗めに

	// オーバーレイ（同じテクスチャで明るい色）
	prompt.overlay_ = std::make_unique<Sprite>();
	prompt.overlay_->Initialize(spriteCommon_, iconPath);
	prompt.overlay_->SetPosition({ kPromptBaseX, y });
	prompt.overlay_->SetSize({ kIconSize, kIconSize });
	prompt.overlay_->SetColor({ 1, 1, 1, 0 }); // 初期は透明
	prompt.overlay_->SetAnchorPoint({ 0.5f, 0.5f });

	// 説明テキスト
	prompt.description_ = std::make_unique<Sprite>();
	prompt.description_->Initialize(spriteCommon_, descriptionPath);
	prompt.description_->SetPosition({ kPromptBaseX + kIconSize * 0.7f, y });
	prompt.description_->SetSize({ kDescriptionWidth, kDescriptionHeight }); 
	prompt.description_->SetAnchorPoint({ 0.0f, 0.5f });

	prompts_.push_back(std::move(prompt));
}
