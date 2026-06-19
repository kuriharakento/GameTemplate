#include "GravityPhysicsComponent.h"
#include "engine/gameobject/base/GameObject.h"
#include "application/GameObject/Combatable/character/base/Character.h"
#include "time/TimeManager.h"

namespace GameObjectComponent
{
	// コンストラクタ：重力と垂直速度の初期化
	GravityPhysicsComponent::GravityPhysicsComponent(float gravity)
		: gravity_(gravity)
		, verticalVelocity_(0.0f)
	{
	}

	// フレームごとの更新処理
	void GravityPhysicsComponent::Update(::GameObject* owner)
	{
		// キャラクターが接地している場合は処理をスキップ
		auto character = dynamic_cast<::Character*>(owner);
		if (character && character->IsGrounded())
		{
			return;
		}

		// デルタタイムを取得
		float deltaTime = TimeManager::GetInstance().GetGameContext().deltaTime;

		// 重力を垂直速度に適用（下方向は負）
		verticalVelocity_ -= gravity_ * deltaTime;

		// 位置を更新
		auto pos = owner->GetPosition();
		pos.y += verticalVelocity_ * deltaTime;

		// 地面より下に行かせない処理
		if (pos.y < kGroundHeight + owner->GetScale().y)
		{
			pos.y = owner->GetScale().y;
			verticalVelocity_ = 0.0f; // 着地で速度リセット
		}

		owner->SetPosition(pos);
	}
}