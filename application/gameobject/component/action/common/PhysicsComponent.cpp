#include "PhysicsComponent.h"
#include "engine/gameobject/base/GameObject.h"
#include "engine/time/TimeManager.h"
#include "engine/gameobject/component/base/ComponentFactory.h"
#include <cmath>
#include <algorithm>

// ファクトリに登録
REGISTER_COMPONENT(PhysicsComponent)

namespace GameObjectComponent
{
	PhysicsComponent::PhysicsComponent(::GameObject* owner)
	{
		// メンバ変数をJSONエディタ/シリアライズ用に登録
		Register("movementVelocity", &movementVelocity_);
		Register("externalVelocity", &externalVelocity_);
		Register("externalForce", &externalForce_);
		Register("constantExternalForce", &constantExternalForce_);
		Register("mass", &mass_);
		Register("externalFriction", &externalFriction_);
		Register("externalAirResistance", &externalAirResistance_);
		Register("useGravity", &useGravity_);
		Register("isGrounded", &isGrounded_);
		Register("gravity", &gravity_);
	}

	PhysicsComponent::~PhysicsComponent()
	{
	}

	void PhysicsComponent::Update(::GameObject* owner)
	{
		float dt = TimeManager::GetInstance().GetGameContext().deltaTime;
		if (dt <= 0.0f)
		{
			return;
		}

		// 1. 外部の力から加速度を計算 (F = ma => a = F/m)
		Vector3 totalForce = constantExternalForce_ + externalForce_;
		Vector3 acceleration = totalForce / (mass_ > 0.0f ? mass_ : 1.0f);

		// 2. 加速度を外部物理速度に適用
		externalVelocity_ += acceleration * dt;

		// 重力の適用（外部物理速度のY軸に適用）
		if (useGravity_)
		{
			if (!isGrounded_)
			{
				externalVelocity_.y -= gravity_ * dt;
			}
			else
			{
				// 接地中は下向きに極小の速度（吸着バイアス）を与えることで、
				// 毎フレーム確実にコライダーが地面と交差し、接地判定が安定するようにする
				externalVelocity_.y = -0.1f;
			}
		}

		// 3. 摩擦・空気抵抗の適用 (外部物理速度のXZ軸を減衰)
		float currentDrag = isGrounded_ ? externalFriction_ : externalAirResistance_;
		float dragFactor = std::pow(currentDrag, dt * 60.0f);
		externalVelocity_.x *= dragFactor;
		externalVelocity_.z *= dragFactor;

		// 4. 座標の更新 (自己移動速度 + 外部物理速度 を合算)
		Vector3 finalVelocity = movementVelocity_ + externalVelocity_;
		Vector3 currentPos = owner->GetPosition();
		currentPos += finalVelocity * dt;
		owner->SetPosition(currentPos);

		// 瞬間的な外力と接地状態をリセット
		externalForce_ = { 0.0f, 0.0f, 0.0f };
		isGrounded_ = false;
	}

	void PhysicsComponent::AddExternalForce(const Vector3& force)
	{
		if (mass_ > 0.0f)
		{
			externalForce_ += force;
		}
	}
}
