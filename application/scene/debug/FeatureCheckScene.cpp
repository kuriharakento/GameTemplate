#include "FeatureCheckScene.h"

#include <cmath>
#include <numbers>

#include "engine/scene/factory/SceneFactory.h"
#include "gameobject/manager/GameObjectManager.h"
#include "graphics/atmosphere/BeamRenderer.h"
#include "graphics/atmosphere/FogRenderer.h"
#include "manager/graphics/LineManager.h"
#include "manager/scene/CameraManager.h"
#include "manager/scene/LightManager.h"
#include "math/VectorColorCodes.h"
#include "scene/manager/SceneManager.h"
#include "sequencer/editor/SequencerEditor.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "manager/editor/DebugUIManager.h"
#endif

REGISTER_SCENE(FeatureCheckScene);

namespace
{
// 床（キューブを平たく潰して使う。plane の向きに依存しないため）
constexpr KCE::Vector3 kFloorPosition = { 0.0f, -0.1f, 0.0f };
constexpr KCE::Vector3 kFloorScale = { 20.0f, 0.1f, 20.0f };

// トゥーン・リム比較用のキューブ。左から右へ効きを強くしていく
constexpr float kShadingCubeSpacing = 4.0f;
constexpr float kShadingCubeHeight = 1.0f;
constexpr float kShadingCubeZ = 0.0f;
constexpr KCE::Vector3 kShadingCubeScale = { 1.0f, 1.0f, 1.0f };

// 半透明キューブ。前後に重ねて、カメラを回すと前後が入れ替わるようにしておく
constexpr KCE::Vector3 kTransparentFrontPosition = { -0.6f, 1.0f, 5.0f };
constexpr KCE::Vector3 kTransparentBackPosition = { 0.6f, 1.0f, 6.0f };
constexpr KCE::Vector3 kTransparentScale = { 1.0f, 1.0f, 1.0f };
constexpr KCE::Vector3 kTransparentFrontRgb = { 1.0f, 0.2f, 0.2f };
constexpr KCE::Vector3 kTransparentBackRgb = { 0.2f, 0.4f, 1.0f };

// 1.0 を超える色。LDR だと白で頭打ちになり、HDR ならブルームとトーンマップで差が出る
constexpr KCE::Vector3 kHdrCubePosition = { 0.0f, 1.0f, -5.0f };
constexpr KCE::Vector4 kHdrCubeColor = { 6.0f, 3.0f, 1.0f, 1.0f };

// ステージ上のスポットライト。真上から床へ向けて、ビームが見えるように少し傾ける
constexpr const char* kSpotLightNames[] = { "FeatureCheckSpot0", "FeatureCheckSpot1", "FeatureCheckSpot2" };
constexpr KCE::Vector4 kSpotLightColors[] = {
	{ 1.0f, 0.3f, 0.3f, 1.0f },
	{ 0.3f, 1.0f, 0.3f, 1.0f },
	{ 0.3f, 0.3f, 1.0f, 1.0f },
};
constexpr float kSpotLightHeight = 8.0f;
constexpr float kSpotLightTiltZ = 0.3f;
constexpr float kSpotLightIntensity = 6.0f;
constexpr float kSpotLightDistance = 15.0f;
// コーンの半角（ラジアン）。数式由来なのでここで角度から余弦を作る
constexpr float kSpotLightHalfAngle = std::numbers::pi_v<float> / 8.0f;
constexpr float kSpotLightFalloffHalfAngle = std::numbers::pi_v<float> / 10.0f;

// デバッグカメラの初期位置と向き
constexpr KCE::Vector3 kDebugCameraPosition = { 0.0f, 8.0f, -18.0f };
constexpr KCE::Vector3 kDebugCameraRotation = { 0.35f, 0.0f, 0.0f };

// グリッド
constexpr float kGridSize = 20.0f;
constexpr float kGridSpacing = 1.0f;

// フレームワークが持っているフォグ・ビームは、シーケンサのバインディング経由でしか取れない
KCE::FogRenderer* GetFogRenderer()
{
	if (!KCE::SequencerEditor::HasInstance())
	{
		return nullptr;
	}
	return KCE::SequencerEditor::GetInstance()->GetPlayer().GetBindingContext().GetFogRenderer();
}

KCE::BeamRenderer* GetBeamRenderer()
{
	if (!KCE::SequencerEditor::HasInstance())
	{
		return nullptr;
	}
	return KCE::SequencerEditor::GetInstance()->GetPlayer().GetBindingContext().GetBeamRenderer();
}
} // namespace

void FeatureCheckScene::Initialize()
{
#ifdef USE_IMGUI
	KCE::DebugUIManager::GetInstance()->RegisterDebugUI(this, "Feature Check", [this]() { this->DrawImGui(); }, KCE::DebugUIArea::Inspector);
#endif

	// 床
	floor_ = CreateObject("FeatureCheck_Floor", kFloorPosition, kFloorScale);

	// トゥーン・リムの比較。0 / 0.5 / 1 の3段階で並べる
	for (size_t i = 0; i < kShadingCubeCount; ++i)
	{
		const float offset = (static_cast<float>(i) - static_cast<float>(kShadingCubeCount - 1) * 0.5f) * kShadingCubeSpacing;
		const std::string name = "FeatureCheck_Shading" + std::to_string(i);
		shadingCubes_[i] = CreateObject(name, { offset, kShadingCubeHeight, kShadingCubeZ }, kShadingCubeScale);

		const float amount = static_cast<float>(i) / static_cast<float>(kShadingCubeCount - 1);
		if (auto* renderable = shadingCubes_[i]->GetRenderable3d())
		{
			renderable->SetToonAmount(amount);
			renderable->SetRimStrength(amount);
		}
	}

	// 半透明。深度を書かないので、重なり方とソート順が目で分かる
	transparentFront_ = CreateObject("FeatureCheck_TransparentFront", kTransparentFrontPosition, kTransparentScale);
	transparentBack_ = CreateObject("FeatureCheck_TransparentBack", kTransparentBackPosition, kTransparentScale);
	for (auto* object : { transparentFront_.get(), transparentBack_.get() })
	{
		if (auto* renderable = object->GetRenderable3d())
		{
			renderable->SetRenderQueue(KCE::RenderQueue::Transparent);
		}
	}
	ApplyTransparentColors();

	// HDR。ライティングを切って色をそのまま出す。G-Buffer は色を LDR で持つ可能性があるので Forward にしておく
	hdrCube_ = CreateObject("FeatureCheck_HDR", kHdrCubePosition, kShadingCubeScale);
	if (auto* renderable = hdrCube_->GetRenderable3d())
	{
		renderable->SetRenderingType(KCE::RenderingType::Forward);
		renderable->SetEnableLighting(false);
		renderable->SetColor(kHdrCubeColor);
	}

	SetupStageLights();

	// デバッグカメラ
	debugCamera_ = std::make_unique<KCE::DebugCamera>();
	debugCamera_->Initialize(sceneManager_->GetCameraManager()->GetActiveCamera());
	debugCamera_->Start(kDebugCameraPosition, kDebugCameraRotation);
}

std::unique_ptr<KCE::GameObject> FeatureCheckScene::CreateObject(const std::string& name, const KCE::Vector3& position, const KCE::Vector3& scale)
{
	auto object = std::make_unique<KCE::GameObject>("FeatureCheck");
	object->Initialize(sceneManager_->GetObject3dCommon(), sceneManager_->GetLightManager());
	object->SetName(name);
	object->SetPosition(position);
	object->SetScale(scale);

	// GameObjectManager には非所有で登録するだけ。外すのは OnFinalize
	KCE::GameObjectManager::GetInstance()->Register(object.get());
	return object;
}

void FeatureCheckScene::SetupStageLights()
{
	KCE::LightManager* lightManager = sceneManager_->GetLightManager();
	if (!lightManager)
	{
		return;
	}

	const float cosAngle = std::cos(kSpotLightHalfAngle);
	const float cosFalloffStart = std::cos(kSpotLightFalloffHalfAngle);

	for (size_t i = 0; i < kSpotLightCount; ++i)
	{
		const std::string name = kSpotLightNames[i];
		const float x = (static_cast<float>(i) - static_cast<float>(kSpotLightCount - 1) * 0.5f) * kShadingCubeSpacing;

		lightManager->AddSpotLight(name);
		lightManager->SetSpotLightPosition(name, { x, kSpotLightHeight, 0.0f });
		lightManager->SetSpotLightDirection(name, KCE::Vector3::Normalize({ 0.0f, -1.0f, kSpotLightTiltZ }));
		lightManager->SetSpotLightColor(name, kSpotLightColors[i]);
		lightManager->SetSpotLightIntensity(name, kSpotLightIntensity);
		lightManager->SetSpotLightDistance(name, kSpotLightDistance);
		lightManager->SetSpotLightCosAngle(name, cosAngle);
		lightManager->SetSpotLightCosFalloffStart(name, cosFalloffStart);
	}

	// フォグとビームは既定で無効なので、ここでオンにする。元の値は抜けるときに戻す
	if (auto* fog = GetFogRenderer())
	{
		prevFogEnabled_ = fog->GetSettings().enabled;
		fog->GetSettings().enabled = true;
	}
	if (auto* beam = GetBeamRenderer())
	{
		prevBeamEnabled_ = beam->GetSettings().enabled;
		beam->GetSettings().enabled = true;
		for (const char* name : kSpotLightNames)
		{
			beam->SetBeamEnabled(name, true);
		}
	}
}

void FeatureCheckScene::ApplyTransparentColors()
{
	if (transparentFront_)
	{
		transparentFront_->SetColor({ kTransparentFrontRgb.x, kTransparentFrontRgb.y, kTransparentFrontRgb.z, transparentAlpha_ });
	}
	if (transparentBack_)
	{
		transparentBack_->SetColor({ kTransparentBackRgb.x, kTransparentBackRgb.y, kTransparentBackRgb.z, transparentAlpha_ });
	}
}

void FeatureCheckScene::RestoreAtmosphere()
{
	if (auto* fog = GetFogRenderer())
	{
		fog->GetSettings().enabled = prevFogEnabled_;
	}
	if (auto* beam = GetBeamRenderer())
	{
		beam->GetSettings().enabled = prevBeamEnabled_;
		for (const char* name : kSpotLightNames)
		{
			beam->SetBeamEnabled(name, false);
		}
	}

	// LightManager には個別削除が無いので、明るさ0にして他のシーンに影響させない
	if (KCE::LightManager* lightManager = sceneManager_->GetLightManager())
	{
		for (const char* name : kSpotLightNames)
		{
			lightManager->SetSpotLightIntensity(name, 0.0f);
			lightManager->SetSpotLightShadowEnabled(name, false);
		}
	}
}

void FeatureCheckScene::OnFinalize()
{
	RestoreAtmosphere();

	// 破棄の前に登録を外す（GameObjectManager に死んだポインタを残さない）
	auto* manager = KCE::GameObjectManager::GetInstance();
	for (auto* object : { floor_.get(), transparentFront_.get(), transparentBack_.get(), hdrCube_.get() })
	{
		manager->Unregister(object);
	}
	for (auto& cube : shadingCubes_)
	{
		manager->Unregister(cube.get());
	}

	floor_.reset();
	transparentFront_.reset();
	transparentBack_.reset();
	hdrCube_.reset();
	for (auto& cube : shadingCubes_)
	{
		cube.reset();
	}
}

void FeatureCheckScene::CommonUpdate()
{
	if (debugCamera_ && useDebugCamera_)
	{
		debugCamera_->Update();
	}

	// GameObjectManager の更新はフレームワークから呼ばれないので、ここで回す
	KCE::GameObjectManager::GetInstance()->Update();
}

void FeatureCheckScene::Draw3D()
{
	BaseScene::Draw3D();

	KCE::LineManager::GetInstance()->DrawGrid(kGridSize, kGridSpacing, KCE::VectorColorCodes::White);

	// Forward で描く不透明の GameObject（HDR キューブ）
	KCE::GameObjectManager::GetInstance()->Draw3D(sceneManager_->GetCameraManager());
}

void FeatureCheckScene::DrawGBuffer()
{
	BaseScene::DrawGBuffer();

	// Deferred で描く不透明の GameObject（床・比較用キューブ）
	KCE::GameObjectManager::GetInstance()->DrawGBuffer(sceneManager_->GetCameraManager());
}

void FeatureCheckScene::DrawShadow()
{
	BaseScene::DrawShadow();
	KCE::GameObjectManager::GetInstance()->DrawShadow();
}

void FeatureCheckScene::Draw2D()
{
}

void FeatureCheckScene::DrawImGui()
{
#ifdef USE_IMGUI
	ImGui::TextWrapped("シーケンサで動かすときは、Timeline の Preview Object で FeatureCheck_* を選ぶ。");
	ImGui::TextWrapped("カメラトラックを再生するときは、下のデバッグカメラをオフにする。");

	if (ImGui::Checkbox("Debug Camera", &useDebugCamera_) && debugCamera_)
	{
		if (useDebugCamera_)
		{
			debugCamera_->Start(kDebugCameraPosition, kDebugCameraRotation);
		}
		else
		{
			debugCamera_->Stop();
		}
	}

	ImGui::SeparatorText("Transparent");
	if (ImGui::SliderFloat("Alpha", &transparentAlpha_, 0.0f, 1.0f))
	{
		ApplyTransparentColors();
	}

	ImGui::SeparatorText("Atmosphere");
	if (auto* fog = GetFogRenderer())
	{
		ImGui::Checkbox("Fog", &fog->GetSettings().enabled);
	}
	if (auto* beam = GetBeamRenderer())
	{
		ImGui::Checkbox("Beam", &beam->GetSettings().enabled);
	}
	ImGui::TextDisabled("細かい値は Fog / Beam / Outline / PostProcess の各デバッグUIで調整する。");

	ImGui::SeparatorText("Objects");
	for (size_t i = 0; i < kShadingCubeCount; ++i)
	{
		if (shadingCubes_[i])
		{
			ImGui::BulletText("%s", shadingCubes_[i]->GetName().c_str());
		}
	}
	for (auto* object : { floor_.get(), transparentFront_.get(), transparentBack_.get(), hdrCube_.get() })
	{
		if (object)
		{
			ImGui::BulletText("%s", object->GetName().c_str());
		}
	}
#endif
}
