#include "EnemyUIComponent.h"

#include "engine/gameobject/base/GameObject.h"
#include "application/gameObject/component/action/StatusComponent.h"
#include "base/WinApp.h"

// 体力バーの色
const Vector4 kHealthColorHigh = { 0.8f, 0.2f, 0.2f, 1.0f };    // 赤（敵なので赤系）
const Vector4 kHealthColorMedium = { 1.0f, 0.5f, 0.0f, 1.0f };  // オレンジ
const Vector4 kHealthColorLow = { 0.5f, 0.1f, 0.1f, 1.0f };     // 暗い赤

// 背景色
const Vector4 kHealthBarBgColor = { 0.2f, 0.2f, 0.2f, 0.8f };   // 半透明グレー

namespace GameObjectComponent
{
	EnemyUIComponent::EnemyUIComponent(::SpriteCommon* spriteCommon, ::CameraManager* camera)
		: spriteCommon_(spriteCommon)
		, camera_(camera)
	{
		InitializeUI();
	}

	void EnemyUIComponent::InitializeUI()
	{
		// 体力バー背景
		healthBarBg_ = std::make_unique<::GameUI>();
		healthBarBg_->Initialize(spriteCommon_, "./Resources/UI/hp_bar_frame.png");
		healthBarBg_->SetSize({ healthBarWidth_, healthBarHeight_ });
		healthBarBg_->SetAnchorPoint({ 0.0f, 0.5f });
		healthBarBg_->SetInteractable(false);
		healthBarBg_->SetColor(kHealthBarBgColor);

		// 体力バー
		healthBarFill_ = std::make_unique<::GameUI>();
		healthBarFill_->Initialize(spriteCommon_, "./Resources/UI/hp_bar_fill.png");
		healthBarFill_->SetSize({ healthBarWidth_, healthBarHeight_ });
		healthBarFill_->SetAnchorPoint({ 0.0f, 0.5f });
		healthBarFill_->SetInteractable(false);
		healthBarFill_->SetColor(kHealthColorHigh);
	}

	void EnemyUIComponent::CacheComponents(::GameObject* owner)
	{
		if (isInitialized_) return;

		statusComp_ = owner->GetComponent<StatusComponent>();
		isInitialized_ = true;
	}

	void EnemyUIComponent::Update(::GameObject* owner)
	{
		CacheComponents(owner);

		// 体力バーの更新
		if (isHealthBarVisible_)
		{
			UpdateHealthBar(owner);
			healthBarBg_->Update();
			healthBarFill_->Update();
		}
	}

	void EnemyUIComponent::Draw2D()
	{
		// 体力バー
		if (isHealthBarVisible_)
		{
			healthBarBg_->Draw();
			healthBarFill_->Draw();
		}
	}

	void EnemyUIComponent::UpdateHealthBar(::GameObject* owner)
	{
		if (!camera_) return;

		auto status = statusComp_.lock();
		if (!status) return;

		// 敵の死亡時は非表示
		if (!status->isAlive)
		{
			healthBarBg_->SetVisible(false);
			healthBarFill_->SetVisible(false);
			return;
		}

		healthBarBg_->SetVisible(true);
		healthBarFill_->SetVisible(true);

		// 敵のワールド座標を取得（ワールドオフセットを加算）
		Vector3 enemyPos = owner->GetPosition();
		Vector3 worldPos = enemyPos + healthBarOffset_;

		// アクティブカメラを取得
		::Camera* camera = camera_->GetActiveCamera();
		if (!camera) return;

		// ワールド座標をスクリーン座標に変換
		Vector2 screenPos = WorldToScreen(worldPos, camera);

		// スクリーン座標でオフセットを加える（画面上で上方向に表示するため）
		screenPos.x += screenOffset_.x;
		screenPos.y += screenOffset_.y;

		// スクリーン座標を直接設定
		healthBarBg_->SetScreenPosition(screenPos);
		healthBarFill_->SetScreenPosition(screenPos);

		// HP比率を計算
		float hpRatio = status->hp.GetValue() / status->maxHp.GetValue();
		if (hpRatio < 0.0f)
		{
			hpRatio = 0.0f;
		}
		if (hpRatio > 1.0f)
		{
			hpRatio = 1.0f;
		}

		// 体力バーの幅を更新
		float fillWidth = healthBarWidth_ * hpRatio;
		healthBarFill_->SetSize({ fillWidth, healthBarHeight_ });

		// HP残量に応じて色を変更
		if (hpRatio > kHpThresholdHigh)
		{
			healthBarFill_->SetColor(kHealthColorHigh);
		}
		else if (hpRatio > kHpThresholdLow)
		{
			healthBarFill_->SetColor(kHealthColorMedium);
		}
		else
		{
			healthBarFill_->SetColor(kHealthColorLow);
		}
	}

	Vector2 EnemyUIComponent::WorldToScreen(const Vector3& worldPosition, ::Camera* camera) const
	{
		if (!camera)
		{
			return Vector2(0.0f, 0.0f);
		}

		// ビュープロジェクション行列を取得
		Matrix4x4 viewProjectionMatrix = camera->GetViewProjectionMatrix();

		// ビューポート行列を作成
		Matrix4x4 viewportMatrix = MakeViewportMatrix(
			0.0f,
			0.0f,
			kScreenWidth,
			kScreenHeight,
			0.0f,
			1.0f
		);

		// ワールド座標を同次座標に変換
		Vector3 screenPos = MathUtils::Transform(worldPosition, viewProjectionMatrix);

		// 透視除算
		if (screenPos.z != 0.0f)
		{
			screenPos.x /= screenPos.z;
			screenPos.y /= screenPos.z;
		}

		// ビューポート変換
		screenPos = MathUtils::Transform(screenPos, viewportMatrix);

		return Vector2(screenPos.x, screenPos.y);
	}

	void EnemyUIComponent::SetHealthBarSize(float width, float height)
	{
		healthBarWidth_ = width;
		healthBarHeight_ = height;

		if (healthBarBg_)
		{
			healthBarBg_->SetSize({ width, height });
		}
		if (healthBarFill_)
		{
			healthBarFill_->SetSize({ width, height });
		}
	}
}