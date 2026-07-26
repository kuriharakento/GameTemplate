#pragma once
#include <memory>

// scene
#include "engine/scene/interface/BaseScene.h"
#include "graphics/3d/Object3d.h"

/**
 * @brief 汎用タイトル/サンプルシーン。
 * 
 * テンプレート用の基本シーン実装。
 */
class TitleScene : public BaseScene
{
public:
    /**
     * @brief 初期化。
     */
    void Initialize() override;
    
    /**
     * @brief 3D描画。
     */
    void Draw3D() override;
    
    /**
     * @brief 2D描画。
     */
    void Draw2D() override;
    
    /**
     * @brief ImGuiデバッグUI。
     */
    void DrawImGui() override;

protected:
    void OnFinalize() override;
	void CommonUpdate() override;

private:
	// 3D背景・サンプルオブジェクト
	std::unique_ptr<Object3d> skydome_;
	std::unique_ptr<Object3d> sampleCube_;

	// アニメーション用タイマー
	float timer_ = 0.0f;
};



