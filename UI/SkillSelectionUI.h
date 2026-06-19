#pragma once
#include <memory>
#include <functional>
#include <string>
#include <cstdint>
#include <vector>
#include "graphics/2d/SpriteCommon.h"

class GameUI;
class FontSprite;

/**
 * @brief スコア向上やスキル強化の選択肢を表示するアイテム情報
 */
struct UpgradeOption
{
    std::string title;
    std::string description;
    std::function<void()> onSelect;
    std::string texturePath; // 追加: 専用アーカイブテクスチャのパス
    std::string titleTexturePath; // 追加: タイトル画像のパス
    uint32_t color = 0xFFFFFFFF; // RGBA
};

/**
 * @brief レベルアップ時にスキルの選択肢を表示するUIクラス。
 *
 * - 最大3枚のカードを表示
 * - 1/2/3キー or マウスクリックで選択
 * - ホバーでカード拡大演出
 */
class SkillSelectionUI
{
public:
	void Initialize(SpriteCommon* spriteCommon);
	void Update();
	void Draw();

	/**
	 * @brief 通常のスキルルート選択 (LV2/3用)
	 */
	void Show(uint32_t level, const std::string& optionA, const std::string& optionB,
		std::function<void(int)> onSelect, const std::string& texA = "", const std::string& texB = "");

	/**
	 * @brief 多彩なアップグレード選択 (LV4+用)
	 */
	void ShowUpgrades(const std::vector<UpgradeOption>& options);

	// 選択中かどうか
	bool IsActive() const { return isActive_; }

private:
	void HideAll();

	SpriteCommon* spriteCommon_ = nullptr;
	bool isActive_ = false;
	float inputLockoutTimer_ = 0.0f;
	static constexpr float kInputLockoutTime = 0.5f;

	// 選択肢の情報
    struct CardInstance {
        std::unique_ptr<GameUI> bg;
        std::unique_ptr<GameUI> icon; // 追加: スキルアイコン
        std::unique_ptr<GameUI> titleIcon; // 追加: タイトル画像
        std::string title;
        std::string desc;
        float scale = 1.0f;
        std::function<void()> onSelect;
    };
    std::vector<CardInstance> cards_;

	// UIパーツ
	std::unique_ptr<GameUI> overlay_;
	std::unique_ptr<FontSprite> mainTitleFont_;

	// レイアウト定数
	static constexpr float kCardWidth = 320.0f;
	static constexpr float kCardHeight = 400.0f;
	static constexpr float kCardGap = 40.0f;
	static constexpr float kCardY = 360.0f;

	// ホバー演出定数
	static constexpr float kHoverScale = 1.05f;
	static constexpr float kNormalScale = 1.0f;
	static constexpr float kScaleLerpSpeed = 0.2f;

	// フォントスケール定数
	static constexpr float kMainTitleScale = 0.3f;
	static constexpr float kCardTitleScale = 0.2f;
	static constexpr float kCardDescScale = 0.16f;
};
