#include "Character.h"

void Character::Update()
{
	// 無敵時間の更新
	if (isInvincible_)
	{
		invincibleTimer_ -= 1.0f / 60.0f; // フレームレートに応じて減少
		if (invincibleTimer_ <= 0.0f)
		{
			invincibleTimer_ = 0.0f;
			isInvincible_ = false; // 無敵状態解除
		}
	}

	// 基底クラスの更新処理を呼び出す
	GameObject::Update();
}

void Character::AddComponent(const std::string& name, std::unique_ptr<GameObjectComponent::IGameObjectComponent> comp)
{
	if (auto collider = dynamic_cast<GameObjectComponent::ICollisionComponent*>(comp.get()))
	{
		// 衝突判定コンポーネントの場合は、衝突時の処理を設定
		CollisionSettings(collider);
	}
	// コンポーネントを追加
	GameObject::AddComponent(name, std::move(comp));
}

void Character::SetInvincible(float duration)
{
	isInvincible_ = true;
	invincibleTimer_ = duration; // 無敵時間を設定
}
