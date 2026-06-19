#pragma once
#include "application/gameObject/combatable/character/base/Character.h"
#include "engine/gameobject/component/base/ICollisionComponent.h"
#include "graphics/2d/SpriteCommon.h"

class EnemyManager;
namespace GameObjectComponent
{
	class WeaponManagerComponent;
	class IWeaponComponent;
}
class PostProcessManager;

class Player : public Character
{
public:
	~Player() override;
	Player(const std::string& tag = gameObjectTag::character::Player);
	void Initialize(Object3dCommon* object3dCommon, SpriteCommon* spriteCommon, LightManager* lightManager, EnemyManager* enemyManager, CameraManager* camera, PostProcessManager* postProcessManager = nullptr);
	void TakeDamage(float damage) override;

	/**
	 * @brief 武器管理コンポーネントを取得
	 * @return 武器管理コンポーネントへのポインタ
	 */
	GameObjectComponent::WeaponManagerComponent* GetWeaponManager() const;

	/**
	 * @brief 現在装備中の武器を取得
	 * @return 現在の武器コンポーネント（なければnullptr）
	 */
	GameObjectComponent::IWeaponComponent* GetCurrentWeapon() const;

private:
	void CollisionSettings(GameObjectComponent::ICollisionComponent* collider) override;
};
