#pragma once
#include "camerawork/debug/DebugCamera.h"
#include "scene/interface/BaseScene.h"
#include "effects/particle/editor/ParticleEditor.h"

/**
 * @brief パーティクルテストシーン
 */
class ParticleTestScene : public KCE::BaseScene
{
public:
	void Initialize() override;
	void Draw3D() override;
	void Draw2D() override;

	/**
	 * @brief シーン全体の共通更新処理
	 */
	void CommonUpdate() override;

protected:
	void OnFinalize() override;

private:
	// ライティング
	static constexpr KCE::Vector3 kLightDirection = { 0.0f, -1.0f, 0.0f };
	static constexpr float kLightIntensity = 0.0f;

	std::unique_ptr<KCE::DebugCamera> debugCamera_;
	std::unique_ptr<KCE::ParticleEditor> particleEditor_;
	// スカイドーム（背景天球）
	std::unique_ptr<KCE::Object3d> skydome_;
};
