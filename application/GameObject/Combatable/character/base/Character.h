#pragma once

#include "application/GameObject/combatable/base/CombatableObject.h"
#include "engine/gameobject/component/base/ICollisionComponent.h"
#include "math/Vector3.h"

class Character : public CombatableObject
{
public:
	virtual ~Character() = default;
	explicit Character(const std::string& tag = gameObjectTag::common::Character): CombatableObject(tag){}
	virtual void Update() override;
	void AddComponent(const std::string& name, std::unique_ptr<GameObjectComponent::IGameObjectComponent> comp);

	// トランスフォーム
	const Vector3& GetPosition() const { return transform_.translate; }
	void SetPosition(const Vector3& position) { transform_.translate = position; }
	const Vector3& GetRotation() const { return transform_.rotate; }
	void SetRotation(const Vector3& rotation) { transform_.rotate = rotation; }
	const Vector3& GetScale() const { return transform_.scale; }
	void SetScale(const Vector3& scale) { transform_.scale = scale; }

	// 無敵状態
	void SetInvincible(float duration); // 無敵状態を設定
	bool IsInvincible() const { return isInvincible_; }

	// 操作可能状態
	void SetControllable(bool controllable) { isControllable_ = controllable; }
	bool IsControllable() const { return isControllable_; }

	// 地面に接地しているか
	void SetIsGrounded(bool grounded) { isGrounded_ = grounded; }
	bool IsGrounded() const { return isGrounded_; }

protected:
	// 状態管理
	bool isInvincible_ = false;
	float invincibleTimer_ = 0.0f; // 無敵時間の残り
	bool isControllable_ = true;   // 操作可能フラグ
	bool isGrounded_ = false; // 地面に接地しているか

private:
	//　当たり判定コンポーネントを追加した際の処理
	virtual void CollisionSettings(GameObjectComponent::ICollisionComponent* collider) {};

};
