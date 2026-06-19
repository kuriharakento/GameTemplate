#pragma once
#include <memory>
#include "math/Vector2.h"
#include <graphics/2d/Sprite.h>

/**
 * @brief レティクル（照準）UI管理クラス
 *
 * マウスカーソルを非表示にし、代わりにスプライトでレティクルを表示します。
 */
class Cursor
{
public:
	/**
	 * @brief コンストラクタ
	 */
	Cursor();

	/**
	 * @brief デストラクタ
	 */
	~Cursor();

	/**
	 * @brief 初期化
	 * @param spriteCommon SpriteCommonへのポインタ
	 * @param texturePath レティクル画像のパス
	 */
	void Initialize(SpriteCommon* spriteCommon, const std::string& texturePath);

	/**
	 * @brief 更新処理（マウス座標を取得してスプライト位置を更新）
	 */
	void Update();

	/**
	 * @brief 描画処理
	 */
	void Draw();

	/**
	 * @brief レティクルの表示/非表示を設定
	 * @param visible true: 表示、false: 非表示
	 */
	void SetVisible(bool visible);

	/**
	 * @brief レティクルが表示されているか取得
	 * @return true: 表示中、false: 非表示
	 */
	bool IsVisible() const { return isVisible_; }

	/**
	 * @brief レティクルのサイズを設定
	 * @param size サイズ（幅・高さ）
	 */
	void SetSize(const Vector2& size);

	/**
	 * @brief レティクルのサイズを取得
	 * @return 現在のサイズ
	 */
	Vector2 GetSize() const;

	/**
	 * @brief レティクルの色を設定
	 * @param color 色（RGBA: 0.0f～1.0f）
	 */
	void SetColor(const Vector4& color);

	/**
	 * @brief レティクルのテクスチャを差し替え
	 * @param texturePath 新しいテクスチャのファイルパス
	 */
	void SetTexture(const std::string& texturePath);

	/**
	 * @brief 現在のマウススクリーン座標を取得
	 * @return マウスのスクリーン座標
	 */
	Vector2 GetMousePosition() const { return mousePosition_; }

private:
	/**
	 * @brief マウスカーソルの表示/非表示を設定
	 * @param show true: 表示、false: 非表示
	 */
	void ShowMouseCursor(bool show);

private:
	// レティクル用スプライト
	std::unique_ptr<Sprite> sprite_;
	// 現在のマウススクリーン座標
	Vector2 mousePosition_;
	// レティクルが表示されているか
	bool isVisible_;
	// マウスカーソルが非表示になっているか
	bool isCursorHidden_;
};