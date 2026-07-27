#pragma once
#include "framework/Framework.h"

class MyGame : public Framework
{
public:
	//初期化
	void Initialize() override;

	//終了
	void Finalize() override;

	//毎フレーム
	void Update()override;

	//描画
	void Draw()override;

protected:
	//ウィンドウサイズ変更時のコールバック
	void OnResize(uint32_t width, uint32_t height) override;
private:
	// =========================
	//  エンジン設定定数
	// =========================

	// シャドウマップ
	static constexpr float kShadowNearPlane = 0.1f;
	static constexpr float kShadowFarPlane = 200.0f;

	// ImGui スタイル
	static constexpr float kImGuiWindowRounding = 0.0f;
	static constexpr float kImGuiWindowBorderSize = 0.0f;

	// =========================
	//  メンバ関数
	// =========================

	//テクスチャの読み込み
	void LoadTextures();
	//モデルの読み込み
	//モデルの読み込み
	void LoadModels();

private:
	// シーン描画用レンダーテクスチャ（ポストプロセス後）
	std::unique_ptr<RenderTexture> sceneRenderTexture_;
};

