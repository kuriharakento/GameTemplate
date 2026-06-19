#pragma once
#include "Obstacle.h"
#include "application/gameObject/base/GameObjectTag.h"

class BarrierBlock : public Obstacle
{
public:
	explicit BarrierBlock(const std::string& tag = gameObjectTag::item::BarrierBlock) : Obstacle(tag) {}
	void Initialize(Object3dCommon* object3dCommon, LightManager* lightManager) override;
	void Update() override;
	void Draw(CameraManager* camera) override;
	void DrawShadow(Camera* camera = nullptr) override;
	void CollisionSettings(GameObjectComponent::ICollisionComponent* collider) override;
};

