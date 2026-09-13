#pragma once
#include <memory>
#include <vector>
#include "scene/interface/BaseScene.h"
#include "gameobject/component/collision/CollisionManager.h"

/** @brief コンポーネント寿命と衝突をまとめて確認するシーン。 */
class DebugScene : public KCE::BaseScene
{
public:
    void Initialize() override;
    void CommonUpdate() override;
    void Draw3D() override;
    void Draw2D() override;
    void DrawImGui() override;
protected:
    void OnFinalize() override;
private:
    KCE::GameObject* CreateObject(const char* name, const KCE::Vector3& position, const KCE::Vector3& scale);
    std::vector<std::unique_ptr<KCE::GameObject>> objects_;
    KCE::CollisionManager::RaycastHit raycastHit_{};
    int colliderEnterCount_ = 0;
	int colliderExitCount_ = 0;
    int objectEnterCount_ = 0;
    int bulletHitCount_ = 0;
    int rayEnterCount_ = 0;
    int spawnedBulletFalseHitCount_ = 0;
    int frameCount_ = 0;
    bool spawnedBullet_ = false;
	bool hasRaycastHit_ = false;
	// objects_ が所有し、シーン終了までは有効。
	KCE::GameObjectComponent::Collider* bodyCollider_ = nullptr;
	int awakeCount_ = 0;
	int enableCount_ = 0;
	int startCount_ = 0;
	int updateCount_ = 0;
	int lateUpdateCount_ = 0;
	int disableCount_ = 0;
	int destroyCount_ = 0;
	// objects_ が所有し、35フレーム目に削除するまでは有効。
	KCE::GameObjectComponent::Component* lifecycleProbe_ = nullptr;
	KCE::GameObject* lifecycleObject_ = nullptr;
};
