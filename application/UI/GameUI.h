#pragma once

#include <functional>
#include <memory>
#include <string>

#include "graphics/2d/Sprite.h"
#include "graphics/2d/SpriteCommon.h"
#include "input/Input.h"
#include "base/Camera.h"
#include "base/WinApp.h"
#include "math/Vector2.h"
#include "math/Vector3.h"

/**
 * @class GameUI
 * @brief ゲーム内のUI要素を管理するクラス
 *
 * スプライトベースのUI要素を管理し、マウスとの当たり判定やコールバック機能を提供します。
 * ワールド座標からスクリーン座標への変換にも対応しています。
 */
class GameUI
{
public:
	/**
	 * @brief コンストラクタ
	 */
	GameUI() = default;

	/**
	 * @brief デストラクタ
	 */
	~GameUI() = default;

	/**
	 * @brief 初期化処理
	 * @param spriteCommon スプライト共通設定
	 * @param textureFilePath テクスチャファイルパス
	 */
	void Initialize(SpriteCommon* spriteCommon, const std::string& textureFilePath);

	/**
	 * @brief 更新処理
	 * @param input 入力管理クラス
	 */
	void Update();

	/**
	 * @brief 描画処理
	 */
	void Draw();

	/*---------------[ セッター ]---------------*/

	/**
	 * @brief スクリーン座標を直接設定
	 * @param screenPosition スクリーン座標
	 */
	void SetScreenPosition(const Vector2& screenPosition);

	/**
	 * @brief ワールド座標を設定し、スクリーン座標に自動変換
	 * @param worldPosition ワールド座標
	 * @param camera カメラ
	 */
	void SetWorldPosition(const Vector3& worldPosition, Camera* camera);

	/**
	 * @brief UIのサイズを設定
	 * @param size サイズ
	 */
	void SetSize(const Vector2& size);

	/**
	 * @brief テクスチャサイズを設定
	 * @param textureSize テクスチャサイズ
	 */
	void SetTextureSize(const Vector2& textureSize);

	/**
	 * @brief UIの色を設定
	 * @param color 色（RGBA）
	 */
	void SetColor(const Vector4& color);

	/**
	 * @brief UIの回転を設定
	 * @param rotation 回転角度（ラジアン）
	 */
	void SetRotation(float rotation);

	/**
	 * @brief アンカーポイントを設定
	 * @param anchorPoint アンカーポイント（0.0〜1.0）
	 */
	void SetAnchorPoint(const Vector2& anchorPoint);

	/**
	 * @brief UIの有効/無効を設定
	 * @param isEnabled 有効かどうか
	 */
	void SetEnabled(bool isEnabled);

	/**
	 * @brief UIの表示/非表示を設定
	 * @param isVisible 表示するかどうか
	 */
	void SetVisible(bool isVisible);

	/**
	 * @brief マウスカーソルとの当たり判定を有効/無効を設定
	 * @param isInteractable インタラクト可能かどうか
	 */
	void SetInteractable(bool isInteractable);

	/**
	 * @brief テクスチャを差し替え
	 * @param textureFilePath テクスチャファイルパス
	 */
	void SetTexture(const std::string& textureFilePath);

	/**
	 * @brief マウスカーソルが当たった瞬間に呼ばれるコールバックを設定
	 * @param callback コールバック関数
	 */
	void SetOnHoverEnterCallback(const std::function<void()>& callback);

	/**
	 * @brief マウスカーソルが当たり続けている間呼ばれるコールバックを設定
	 * @param callback コールバック関数
	 */
	void SetOnHoverStayCallback(const std::function<void()>& callback);

	/**
	 * @brief マウスカーソルが離れた瞬間に呼ばれるコールバックを設定
	 * @param callback コールバック関数
	 */
	void SetOnHoverExitCallback(const std::function<void()>& callback);

	/**
	 * @brief クリックされた瞬間に呼ばれるコールバックを設定
	 * @param callback コールバック関数
	 */
	void SetOnClickCallback(const std::function<void()>& callback);

	/**
	 * @brief クリックされている間呼ばれるコールバックを設定
	 * @param callback コールバック関数
	 */
	void SetOnClickHoldCallback(const std::function<void()>& callback);

	/**
	 * @brief クリックが離された瞬間に呼ばれるコールバックを設定
	 * @param callback コールバック関数
	 */
	void SetOnClickReleaseCallback(const std::function<void()>& callback);

	/*---------------[ ゲッター ]---------------*/

	/**
	 * @brief 現在のスクリーン座標を取得
	 * @return スクリーン座標
	 */
	const Vector2& GetScreenPosition() const;

	/**
	 * @brief UIのサイズを取得
	 * @return サイズ
	 */
	const Vector2& GetSize() const;

	/**
	 * @brief マウスカーソルがUI上にあるか取得
	 * @return ホバー中かどうか
	 */
	bool IsHovered() const;

	/**
	 * @brief UIがクリックされているか取得
	 * @return クリック中かどうか
	 */
	bool IsClicked() const;

	/**
	 * @brief UIが有効かどうかを取得
	 * @return 有効かどうか
	 */
	bool IsEnabled() const;

	/**
	 * @brief UIが表示されているかどうかを取得
	 * @return 表示されているかどうか
	 */
	bool IsVisible() const;

private:
	/**
	 * @brief マウスカーソルとの当たり判定
	 * @param mouseX マウスのX座標
	 * @param mouseY マウスのY座標
	 * @return 当たっているかどうか
	 */
	bool CheckMouseCollision(float mouseX, float mouseY) const;

	/**
	 * @brief ワールド座標をスクリーン座標に変換
	 * @param worldPosition ワールド座標
	 * @param camera カメラ
	 * @return スクリーン座標
	 */
	Vector2 WorldToScreen(const Vector3& worldPosition, Camera* camera) const;

	/**
	 * @brief コールバック関数を実行
	 * @param callback コールバック関数
	 */
	void ExecuteCallback(const std::function<void()>& callback);

private:
	// スプライト
	std::unique_ptr<Sprite> sprite_ = nullptr;

	// スプライト共通設定
	SpriteCommon* spriteCommon_ = nullptr;

	// 前フレームのホバー状態
	bool wasHovered_ = false;

	// 前フレームのクリック状態
	bool wasClicked_ = false;

	// 現在のホバー状態
	bool isHovered_ = false;

	// 現在のクリック状態
	bool isClicked_ = false;

	// 有効/無効フラグ
	bool isEnabled_ = true;

	// 表示/非表示フラグ
	bool isVisible_ = true;

	// インタラクト可能フラグ
	bool isInteractable_ = true;

	// コールバック関数
	std::function<void()> onHoverEnterCallback_ = nullptr;

	// ホバー中コールバック関数
	std::function<void()> onHoverStayCallback_ = nullptr;

	// ホバー終了コールバック関数
	std::function<void()> onHoverExitCallback_ = nullptr;

	// クリックコールバック関数
	std::function<void()> onClickCallback_ = nullptr;

	// クリック中コールバック関数
	std::function<void()> onClickHoldCallback_ = nullptr;

	// クリック終了コールバック関数
	std::function<void()> onClickReleaseCallback_ = nullptr;

	// マウスボタンのインデックス（0: 左ボタン）
	static constexpr int kMouseButtonLeft = 0;

	// スクリーン幅（1280x720の仮想スクリーン解像度に固定）
	static inline const float kScreenWidth = 1280.0f;

	// スクリーン高さ（1280x720の仮想スクリーン解像度に固定）
	static inline const float kScreenHeight = 720.0f;
};