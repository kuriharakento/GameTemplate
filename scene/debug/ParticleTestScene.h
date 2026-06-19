#pragma once
#include "camerawork/debug/DebugCamera.h"
#include "scene/interface/BaseScene.h"
#include "effects/particle/editor/ParticleEditor.h"

/**
 * @brief パーティクルテストシーン
 */
class ParticleTestScene : public BaseScene
{
public:
	void Initialize() override;
	void Finalize() override;
	void Draw3D() override;
	void Draw2D() override;

protected:
	void OnEnterPlaying() override;
	void OnUpdatePlaying() override;

private:
	// ライティング
	static constexpr Vector3 kLightDirection = { 0.0f, -1.0f, 0.0f };
	static constexpr float kLightIntensity = 0.0f;

	std::unique_ptr<DebugCamera> debugCamera_;
	std::unique_ptr<ParticleEditor> particleEditor_;
	// スカイドーム（背景天球）
	std::unique_ptr<Object3d> skydome_;
};