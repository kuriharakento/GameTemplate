#pragma once
#include "engine/gameobject/component/base/IActionComponent.h"
#include "application/ui/GameUI.h"
#include "base/WinApp.h"
#include "math/MatrixFunc.h"
#include "math/MathUtils.h"
#include <memory>

class SpriteCommon;
class Camera;

/**
 * @brief 敵のUI表示を担当するコンポーネント
 *
 * 敵の頭上に体力バーをワールド座標ベースで表示する
 */
namespace GameObjectComponent
{
	class StatusComponent;
	class EnemyUIComponent : public IActionComponent
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param spriteCommon スプライト共通設定
		 * @param camera カメラマネージャー（ワールド座標変換用）
		 */
		EnemyUIComponent(::SpriteCommon* spriteCommon, ::CameraManager* camera);

		/**
		 * @brief デストラクタ
		 */
		~EnemyUIComponent() = default;

		/**
		 * @brief 更新処理
		 * @param owner 所有するGameObject
		 */
		void Update(::GameObject* owner) override;

		/**
		 * @brief 描画処理（3D）※このコンポーネントでは何もしない
		 * @param camera カメラマネージャー
		 */
		void Draw3D(::CameraManager* camera) override {}

		/**
		 * @brief 描画処理（2D UI）
		 */
		void Draw2D() override;

		/*---------------[ ゲッター ]---------------*/

		/**
		 * @brief 体力バーの表示状態を取得
		 * @return 表示中ならtrue
		 */
		bool IsHealthBarVisible() const { return isHealthBarVisible_; }

		/**
		 * @brief 体力バーのワールドオフセットを取得
		 * @return ワールド座標でのオフセット
		 */
		const Vector3& GetHealthBarOffset() const { return healthBarOffset_; }

		/**
		 * @brief スクリーン座標でのオフセットを取得
		 * @return スクリーン座標でのオフセット（ピクセル単位）
		 */
		const Vector2& GetScreenOffset() const { return screenOffset_; }

		/**
		 * @brief 体力バーの幅を取得
		 * @return 体力バーの幅
		 */
		float GetHealthBarWidth() const { return healthBarWidth_; }

		/**
		 * @brief 体力バーの高さを取得
		 * @return 体力バーの高さ
		 */
		float GetHealthBarHeight() const { return healthBarHeight_; }

		/*---------------[ セッター ]---------------*/

		/**
		 * @brief 体力バーの表示/非表示を設定
		 * @param isVisible 表示するならtrue
		 */
		void SetHealthBarVisible(bool isVisible) { isHealthBarVisible_ = isVisible; }

		/**
		 * @brief 体力バーのワールドオフセットを設定（敵の頭上位置調整用）
		 * @param offset ワールド座標でのオフセット
		 */
		void SetHealthBarOffset(const Vector3& offset) { healthBarOffset_ = offset; }

		/**
		 * @brief スクリーン座標でのオフセットを設定
		 * @param offset スクリーン座標でのオフセット（ピクセル単位、上方向はマイナス）
		 */
		void SetScreenOffset(const Vector2& offset) { screenOffset_ = offset; }

		/**
		 * @brief 体力バーのサイズを設定
		 * @param width 幅
		 * @param height 高さ
		 */
		void SetHealthBarSize(float width, float height);

		/**
		 * @brief カメラマネージャーを設定
		 * @param camera カメラマネージャー
		 */
		void SetCameraManager(::CameraManager* camera) { camera_ = camera; }

	private:
		// 定数
		// 体力バーのデフォルトサイズ
		static constexpr float kDefaultHealthBarWidth = 120.0f;
		static constexpr float kDefaultHealthBarHeight = 12.0f;

		// デフォルトのワールドオフセット（敵の高さ分）
		static constexpr float kDefaultWorldOffsetY = 2.0f;

		// スクリーン座標でのデフォルトオフセット（ピクセル単位、上方向はマイナス）
		static constexpr float kDefaultScreenOffsetY = -30.0f;

		// HP残量の閾値
		static constexpr float kHpThresholdHigh = 0.5f;
		static constexpr float kHpThresholdLow = 0.25f;

		// スクリーンサイズ（1280x720の仮想スクリーン解像度に固定）
		static inline const float kScreenWidth = 1280.0f;
		static inline const float kScreenHeight = 720.0f;

		/**
		 * @brief UI要素の初期化
		 */
		void InitializeUI();

		/**
		 * @brief 他のコンポーネントへの参照をキャッシュ
		 * @param owner 所有するGameObject
		 */
		void CacheComponents(::GameObject* owner);

		/**
		 * @brief 体力バーの更新
		 * @param owner 所有するGameObject
		 */
		void UpdateHealthBar(::GameObject* owner);

		/**
		 * @brief ワールド座標をスクリーン座標に変換
		 * @param worldPosition ワールド座標
		 * @param camera カメラ
		 * @return スクリーン座標
		 */
		Vector2 WorldToScreen(const Vector3& worldPosition, ::Camera* camera) const;

	private:
		// スプライト共通設定
		::SpriteCommon* spriteCommon_ = nullptr;

		// カメラマネージャー（ワールド座標変換用）
		::CameraManager* camera_ = nullptr;

		// UI要素
		std::unique_ptr<::GameUI> healthBarBg_;    // 体力バー背景
		std::unique_ptr<::GameUI> healthBarFill_;  // 体力バー（可変）

		// キャッシュしたコンポーネント参照
		std::weak_ptr<StatusComponent> statusComp_;

		// 初期化フラグ
		bool isInitialized_ = false;

		// 表示フラグ
		bool isHealthBarVisible_ = true;

		// 体力バーのワールド座標オフセット（敵の位置からの相対位置）
		Vector3 healthBarOffset_ = { 0.0f, kDefaultWorldOffsetY, 0.0f };

		// スクリーン座標でのオフセット（ピクセル単位）
		Vector2 screenOffset_ = { -kDefaultHealthBarWidth / 2.0f, kDefaultScreenOffsetY };

		// 体力バーのサイズ
		float healthBarWidth_ = kDefaultHealthBarWidth;
		float healthBarHeight_ = kDefaultHealthBarHeight;
	};
}