#pragma once
#include "scene/interface/BaseScene.h"
#include "camerawork/debug/DebugCamera.h"
#include "engine/gameobject/base/GameObject.h"

/**
 * @brief ゲームオブジェクトのテストを行うデバッグ用シーン
 */
class TestScene : public BaseScene
{
public:
	void Initialize() override;
	void Finalize() override;
	void Draw3D() override;
	void Draw2D() override;
	void DrawShadow() override;
	void DrawGBuffer() override;

protected:
	void OnUpdatePlaying() override;

private:
	// ディレクショナルライト設定
	static constexpr Vector3 kLightDirection = { 0.0f, -1.0f, 0.0f };
	static constexpr float kLightIntensity = 1.0f;

	std::unique_ptr<DebugCamera> debugCamera_;
	
	// テスト用のゲームオブジェクト
	std::unique_ptr<GameObject> cubeObject_;
	std::unique_ptr<GameObject> groundObject_;
};
