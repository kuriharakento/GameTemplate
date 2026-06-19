#include "Cursor.h"
#include "graphics/2d/Sprite.h"
#include "input/Input.h"
#include "base/WinApp.h"
#include <Windows.h>

Cursor::Cursor()
	: mousePosition_{}
	, isVisible_(true)
	, isCursorHidden_(false)
{
}

Cursor::~Cursor()
{
	// デストラクタでマウスカーソルを再表示
	if (isCursorHidden_)
	{
		ShowMouseCursor(true);
	}
}

void Cursor::Initialize(SpriteCommon* spriteCommon, const std::string& texturePath)
{
	// スプライトの生成と初期化
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize(spriteCommon, texturePath);

	// レティクルの中心を基準点に設定
	sprite_->SetAnchorPoint({ 0.5f, 0.5f });

#ifdef NDEBUG
	// リリース時はマウスカーソルを非表示にする
	ShowMouseCursor(false);
	isCursorHidden_ = true;
#endif
}

void Cursor::Update()
{
	if (!isVisible_)
	{
		return;
	}

	// マウスのスクリーン座標を取得
	Input* input = Input::GetInstance();
	mousePosition_ = input->GetMousePosition();

	// スプライトの位置を更新
	sprite_->SetPosition(mousePosition_);
	sprite_->Update();
}

void Cursor::Draw()
{
	if (!isVisible_ || !sprite_)
	{
		return;
	}

	sprite_->Draw();
}

void Cursor::SetVisible(bool visible)
{
	isVisible_ = visible;

	// 表示/非表示に応じてマウスカーソルの表示も切り替え
	ShowMouseCursor(!visible);
	isCursorHidden_ = visible;
}

void Cursor::SetSize(const Vector2& size)
{
	if (sprite_)
	{
		sprite_->SetSize(size);
	}
}

Vector2 Cursor::GetSize() const
{
	if (sprite_)
	{
		return sprite_->GetSize();
	}
	return {};
}

void Cursor::SetColor(const Vector4& color)
{
	if (sprite_)
	{
		sprite_->SetColor(color);
	}
}

void Cursor::SetTexture(const std::string& texturePath)
{
	if (sprite_)
	{
		sprite_->SetTexture(texturePath);
	}
}

void Cursor::ShowMouseCursor(bool show)
{
	// Inputクラスを通じてマウスカーソルの表示/非表示を切り替え
	Input::GetInstance()->SetMouseVisible(show);
}