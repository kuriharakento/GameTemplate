#include "BulletComponent.h"
// GameObjectの完全な型を使用するために追加
#include "engine/gameobject/base/GameObject.h"
#include "application/gameObject/combatable/weapon/Bullet.h"
#include "time/TimeManager.h"

namespace GameObjectComponent
{
	// コンストラクタ：メンバ変数の初期化
	BulletComponent::BulletComponent()
		: speed_(0.0f), lifetime_(0.0f), timeAlive_(0.0f)
	{
	}

	// 弾丸の初期化
	void BulletComponent::Initialize(const Vector3& direction, float speed, float lifetime)
	{
		direction_ = direction;
		speed_ = speed;
		lifetime_ = lifetime;
		timeAlive_ = 0.0f;
	}

	void BulletComponent::Update(::GameObject* owner)
	{
		float delta = 0.0f;
		if (ignoreTimeScale_)
		{
			delta = TimeManager::GetInstance().GetGameContext().realDeltaTime;
		}
		else
		{
			delta = TimeManager::GetInstance().GetGameContext().deltaTime;
		}

		// 経過時間を更新
		timeAlive_ += delta;

		// Bullet型にダイナミックキャスト
		auto bullet = dynamic_cast<::Bullet*>(owner);

		// 弾の移動処理
		owner->SetPosition(owner->GetPosition() + direction_ * speed_ * delta);

		// ライフタイムを超えたら弾を非アクティブ化
		if (bullet && timeAlive_ >= lifetime_)
		{
			bullet->SetAlive(false);
		}
	}
}