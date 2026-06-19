#pragma once
#include "engine/gameObject/base/GameObject.h"
#include "engine/gameobject/component/base/ICollisionComponent.h"
#include "math/OBB.h"
#include "application/gameObject/base/GameObjectTag.h"

class Obstacle : public GameObject
{
public:
	virtual ~Obstacle() = default;
	explicit Obstacle(const std::string& tag = gameObjectTag::item::Obstacle) : GameObject(tag) {}
	virtual void Initialize(Object3dCommon* object3dCommon, LightManager* lightManager);
	virtual void Update();
	virtual void Draw(CameraManager* camera);
	void AddComponent(const std::string& name, std::unique_ptr<GameObjectComponent::IGameObjectComponent> comp);

protected:
	virtual void CollisionSettings(GameObjectComponent::ICollisionComponent* collider);
	void ResolvePenetration(GameObject* other);
	bool CheckOBBvsOBBMTV(const OBB& obbA, const OBB& obbB, Vector3& mtv) const;
};
