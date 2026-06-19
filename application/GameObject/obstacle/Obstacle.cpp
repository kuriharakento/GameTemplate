#include "Obstacle.h"
#include "engine/gameobject/component/collision/OBBColliderComponent.h"
#include "engine/gameobject/component/collision/CollisionAlgorithm.h"
#include "application/gameObject/combatable/character/base/Character.h"
#include "math/MathUtils.h"
#include "imgui/imgui.h"

void Obstacle::Initialize(Object3dCommon* object3dCommon, LightManager* lightManager)
{
	GameObject::Initialize(object3dCommon, lightManager);

	// OBBコライダーを追加
	AddComponent("OBBColliderComponent", std::make_unique<GameObjectComponent::OBBColliderComponent>(this));
}

void Obstacle::Update()
{
	GameObject::Update();
}

void Obstacle::Draw(CameraManager* camera)
{
	GameObject::Draw3D(camera);
}

void Obstacle::AddComponent(const std::string& name, std::unique_ptr<GameObjectComponent::IGameObjectComponent> comp)
{
	if (auto collider = dynamic_cast<GameObjectComponent::ICollisionComponent*>(comp.get()))
	{
		CollisionSettings(collider);
	}
	GameObject::AddComponent(name, std::move(comp));
}

void Obstacle::CollisionSettings(GameObjectComponent::ICollisionComponent* collider)
{
	// 衝突時の処理を設定
	collider->SetOnEnter([this](::GameObject* other) {
		ResolvePenetration(other);
						 });
	collider->SetOnStay([this](::GameObject* other) {
		ResolvePenetration(other);
						});
}

void Obstacle::ResolvePenetration(::GameObject* other)
{
	auto character = dynamic_cast<Character*>(other);
	if (!character) return;

	auto obstacleColl = GetComponent<GameObjectComponent::OBBColliderComponent>();
	auto otherColl = other->GetComponent<GameObjectComponent::OBBColliderComponent>();
	if (!obstacleColl || !otherColl) return;

	Vector3 mtv;
	// サブステップ対応の MTV 計算を使用して、高速移動（ダッシュ）時のすり抜けを防ぐ
	if (collisionAlgorithm::CheckOBBvsOBBSubstepMTV(
		obstacleColl->GetOBB(), obstacleColl->GetPreviousPosition(),
		otherColl->GetOBB(), otherColl->GetPreviousPosition(), mtv))
	{
		// キャラクターを押し戻す
		other->SetPosition(other->GetPosition() + mtv);

		// MTVのY成分が十分大きい場合のみ接地判定
		if (mtv.y > 0.1f)
		{
			character->SetIsGrounded(true);
		}
	}
}

bool Obstacle::CheckOBBvsOBBMTV(const OBB& obbA, const OBB& obbB, Vector3& mtv) const
{
	// サブステップなしの通常判定として CollisionAlgorithm を呼ぶ
	return collisionAlgorithm::CheckOBBvsOBBMTV(obbA, obbB, mtv);
}