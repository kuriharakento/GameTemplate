#pragma once
#include <memory>
#include <string>
#include <vector>
#include "graphics/2d/FontSprite.h"
#include "graphics/2d/Sprite.h"
#include "graphics/2d/SpriteCommon.h"
#include "engine/ecs/Registry.h"

/**
 * @class ControlsGuide
 * @brief 操作説明UIとプレイヤー情報を表示するクラス
 *
 * プレイヤーのスキル解放状況に応じた操作ガイドと、
 * HPバー、クールタイム演出を一括管理します。
 */
class ControlsGuide
{
public:
	/**
	 * @struct ControlPrompt
	 * @brief 一つの操作ガイド要素（アイコン＋説明＋クールタイム影）
	 */
	struct ControlPrompt
	{
		std::unique_ptr<Sprite> icon_;
		std::unique_ptr<Sprite> overlay_;     // クールタイム用
		std::unique_ptr<Sprite> description_; // 説明テキスト
		std::string actionName_;              // 判定用
		std::string iconPath_;                // テクスチャパス保持
		float y_ = 0.0f;                      // 元のY座標
	};

public:
	/**
	 * @brief 初期化処理
	 * @param spriteCommon スプライト共通設定
	 * @param registry ECSレジストリへのポインタ
	 * @param playerEntity プレイヤーのEntityID
	 */
	void Initialize(SpriteCommon* spriteCommon, Registry* registry, EntityID playerEntity);

	/**
	 * @brief 更新処理
	 * @details ECSからプレイヤー情報を取得し、UIの状態を更新します。
	 */
	void Update();

	/**
	 * @brief 描画処理
	 */
	void Draw();

	/**
	 * @brief 表示/非表示を設定
	 * @param isVisible 表示するかどうか
	 */
	void SetVisible(bool isVisible);

	/**
	 * @brief 表示状態を取得
	 * @return 表示中かどうか
	 */
	bool IsVisible() const;

private:
	/**
	 * @brief スキルプロンプトの追加
	 * @param actionName アクション名（内部キー）
	 * @param iconPath アイコンの画像パス
	 * @param text 説明テキスト
	 */
	void AddPrompt(const std::string& actionName, const std::string& iconPath, const std::string& text);

private:
	// =========================
	//  定数
	// =========================
	// Prompt Layout
	static constexpr float kIconSize = 48.0f;
	static constexpr float kDescriptionWidth = 88.0f;
	static constexpr float kDescriptionHeight = 30.0f;
	static constexpr float kPromptBaseX = 48.0f;   
	static constexpr float kPromptBaseY = 660.0f;  
	static constexpr float kPromptSpacingY = -64.0f; 
	
	// HP Bar
	static constexpr float kHpBarX = 350.0f; 
	static constexpr float kHpBarY = 660.0f;  
	static constexpr float kHpBarWidth = 500.0f; 
	static constexpr float kHpBarHeight = 16.0f; // 16pxに微調整

	// EXP Bar
	static constexpr float kExpBarX = 350.0f;
	static constexpr float kExpBarY = 640.0f; // HPバーの上に配置
	static constexpr float kExpBarWidth = 500.0f;
	static constexpr float kExpBarHeight = 6.0f;

	// =========================
	//  メンバ変数
	// =========================
	SpriteCommon* spriteCommon_ = nullptr;
	Registry* registry_ = nullptr;
	EntityID playerEntity_ = kInvalidEntity;

	// 操作ガイド要素のリスト
	std::vector<ControlPrompt> prompts_;

	// HPバー
	std::unique_ptr<Sprite> hpBarFG_;
	std::unique_ptr<Sprite> hpBarFrame_;

	// EXPバー
	std::unique_ptr<Sprite> expBarFG_;
	std::unique_ptr<Sprite> expBarFrame_;

	// 表示フラグ
	bool isVisible_ = true;
};
