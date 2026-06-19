#include "GameUI.h"
#include "base/WinApp.h"
#include "math/MatrixFunc.h"
#include <math/MathUtils.h>

void GameUI::Initialize(SpriteCommon* spriteCommon, const std::string& textureFilePath)
{
	// スプライト共通設定を保存
	spriteCommon_ = spriteCommon;

	// スプライトを生成
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize(spriteCommon, textureFilePath);

	// デフォルト設定
	sprite_->SetAnchorPoint(Vector2(0.5f, 0.5f));
}

void GameUI::Update()
{
	// 無効または非表示の場合は処理をスキップ
	if (!isEnabled_ || !isVisible_)
	{
		isHovered_ = false;
		isClicked_ = false;
		sprite_->Update();
		return;
	}

	// 前フレームの状態を保存
	wasHovered_ = isHovered_;
	wasClicked_ = isClicked_;

	// インタラクト可能な場合のみ当たり判定を行う
	if (isInteractable_)
	{
		// マウス座標を取得
		float mouseX = Input::GetInstance()->GetMouseX();
		float mouseY = Input::GetInstance()->GetMouseY();

		// マウスとの当たり判定
		isHovered_ = CheckMouseCollision(mouseX, mouseY);

		// クリック判定
		if (isHovered_)
		{
			isClicked_ = Input::GetInstance()->IsMouseButtonPressed(kMouseButtonLeft);
		}
		else
		{
			isClicked_ = false;
		}

		// コールバック実行
		// ホバーエンター（当たった瞬間）
		if (isHovered_ && !wasHovered_)
		{
			ExecuteCallback(onHoverEnterCallback_);
		}

		// ホバー中（当たり続けている）
		if (isHovered_)
		{
			ExecuteCallback(onHoverStayCallback_);
		}

		// ホバーエグジット（離れた瞬間）
		if (!isHovered_ && wasHovered_)
		{
			ExecuteCallback(onHoverExitCallback_);
		}

		// クリック（押した瞬間）
		if (isClicked_ && !wasClicked_)
		{
			ExecuteCallback(onClickCallback_);
		}

		// クリック中（押し続けている）
		if (isClicked_)
		{
			ExecuteCallback(onClickHoldCallback_);
		}

		// クリックリリース（離した瞬間）
		if (!isClicked_ && wasClicked_)
		{
			ExecuteCallback(onClickReleaseCallback_);
		}
	}
	else
	{
		isHovered_ = false;
		isClicked_ = false;
	}

	// スプライトの更新
	sprite_->Update();
}

void GameUI::Draw()
{
	// 表示フラグが立っている場合のみ描画
	if (isVisible_ && sprite_)
	{
		sprite_->Draw();
	}
}

void GameUI::SetScreenPosition(const Vector2& screenPosition)
{
	if (sprite_)
	{
		sprite_->SetPosition(screenPosition);
	}
}

void GameUI::SetWorldPosition(const Vector3& worldPosition, Camera* camera)
{
	if (sprite_ && camera)
	{
		// ワールド座標をスクリーン座標に変換
		Vector2 screenPos = WorldToScreen(worldPosition, camera);
		sprite_->SetPosition(screenPos);
	}
}

void GameUI::SetSize(const Vector2& size)
{
	if (sprite_)
	{
		sprite_->SetSize(size);
	}
}

void GameUI::SetTextureSize(const Vector2& textureSize)
{
	if (sprite_)
	{
		sprite_->SetTextureSize(textureSize);
	}
}

void GameUI::SetColor(const Vector4& color)
{
	if (sprite_)
	{
		sprite_->SetColor(color);
	}
}

void GameUI::SetRotation(float rotation)
{
	if (sprite_)
	{
		sprite_->SetRotation(rotation);
	}
}

void GameUI::SetAnchorPoint(const Vector2& anchorPoint)
{
	if (sprite_)
	{
		sprite_->SetAnchorPoint(anchorPoint);
	}
}

void GameUI::SetEnabled(bool isEnabled)
{
	isEnabled_ = isEnabled;
}

void GameUI::SetVisible(bool isVisible)
{
	isVisible_ = isVisible;
}

void GameUI::SetInteractable(bool isInteractable)
{
	isInteractable_ = isInteractable;
}

void GameUI::SetTexture(const std::string& textureFilePath)
{
	if (sprite_)
	{
		sprite_->SetTexture(textureFilePath);
	}
}

void GameUI::SetOnHoverEnterCallback(const std::function<void()>& callback)
{
	onHoverEnterCallback_ = callback;
}

void GameUI::SetOnHoverStayCallback(const std::function<void()>& callback)
{
	onHoverStayCallback_ = callback;
}

void GameUI::SetOnHoverExitCallback(const std::function<void()>& callback)
{
	onHoverExitCallback_ = callback;
}

void GameUI::SetOnClickCallback(const std::function<void()>& callback)
{
	onClickCallback_ = callback;
}

void GameUI::SetOnClickHoldCallback(const std::function<void()>& callback)
{
	onClickHoldCallback_ = callback;
}

void GameUI::SetOnClickReleaseCallback(const std::function<void()>& callback)
{
	onClickReleaseCallback_ = callback;
}

const Vector2& GameUI::GetScreenPosition() const
{
	static Vector2 defaultPosition = { 0.0f, 0.0f };
	return sprite_ ? sprite_->GetPosition() : defaultPosition;
}

const Vector2& GameUI::GetSize() const
{
	static Vector2 defaultSize = { 0.0f, 0.0f };
	return sprite_ ? sprite_->GetSize() : defaultSize;
}

bool GameUI::IsHovered() const
{
	return isHovered_;
}

bool GameUI::IsClicked() const
{
	return isClicked_;
}

bool GameUI::IsEnabled() const
{
	return isEnabled_;
}

bool GameUI::IsVisible() const
{
	return isVisible_;
}

bool GameUI::CheckMouseCollision(float mouseX, float mouseY) const
{
	if (!sprite_)
	{
		return false;
	}

	// スプライトの位置とサイズを取得
	Vector2 position = sprite_->GetPosition();
	Vector2 size = sprite_->GetSize();
	Vector2 anchorPoint = sprite_->GetAnchorPoint();

	// アンカーポイントを考慮した矩形の左上座標を計算
	float left = position.x - size.x * anchorPoint.x;
	float top = position.y - size.y * anchorPoint.y;
	float right = left + size.x;
	float bottom = top + size.y;

	// マウス座標が矩形内にあるかチェック
	return (mouseX >= left && mouseX <= right && mouseY >= top && mouseY <= bottom);
}

Vector2 GameUI::WorldToScreen(const Vector3& worldPosition, Camera* camera) const
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

void GameUI::ExecuteCallback(const std::function<void()>& callback)
{
	// コールバックが設定されている場合のみ実行
	if (callback)
	{
		callback();
	}
}