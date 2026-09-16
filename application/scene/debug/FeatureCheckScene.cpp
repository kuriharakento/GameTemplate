#include "FeatureCheckScene.h"

#include <cmath>
#include <numbers>

#include "engine/scene/factory/SceneFactory.h"
#include "gameobject/manager/GameObjectManager.h"
#include "graphics/atmosphere/BeamRenderer.h"
#include "graphics/atmosphere/FogRenderer.h"
#include "graphics/2d/TextOverlay.h"
#include "graphics/text/Text3DRenderer.h"
#include "graphics/atmosphere/VolumetricLightRenderer.h"
#include "graphics/postfx/DepthOfFieldRenderer.h"
#include "graphics/view/PlanarReflection.h"
#include "manager/graphics/LineManager.h"
#include "manager/graphics/ShadowMapManager.h"
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

// 1.0 を超える色。LDR だと白で頭打ちになり、HDR ならブルームとトーンマップで差が出る。
// 緑と青まで明るくするとトーンマップ後にクリーム色になるので、赤だけ大きく超えさせてオレンジに見せる
constexpr KCE::Vector3 kHdrCubePosition = { 0.0f, 1.0f, -5.0f };
constexpr KCE::Vector4 kHdrCubeColor = { 4.0f, 0.45f, 0.05f, 1.0f };

// 床とトゥーン比較用キューブの色。真っ白だと光の当たり方や塗り分けの段が見えないので色を付ける
constexpr KCE::Vector4 kFloorColor = { 0.3f, 0.3f, 0.32f, 1.0f };
constexpr KCE::Vector4 kShadingCubeColor = { 0.35f, 0.6f, 1.0f, 1.0f };
constexpr float kShadingCubeOutlineStrength = 1.0f;

// 奥の画面（モニター）。plane（XY 平面の 2x2）を 16:9 に伸ばす。HDR キューブを寄りで映す。
// キューブは面ごとに UV の一部しか使わないので、映像を丸ごと貼るには向かない
constexpr const char* kMonitorScreenModel = "plane";
constexpr KCE::Vector3 kMonitorScreenPosition = { 0.0f, 2.4f, 9.0f };
constexpr KCE::Vector3 kMonitorScreenScale = { 1.2f, 0.675f, 1.0f };
// plane の表は +Z 向き。Y で半回転させて手前（カメラ側）に表を向ける。左右の反転もこれで直る
constexpr KCE::Vector3 kMonitorScreenRotation = { 0.0f, std::numbers::pi_v<float>, 0.0f };
constexpr KCE::Vector4 kMonitorScreenColor = { 1.0f, 1.0f, 1.0f, 1.0f };
constexpr uint32_t kMonitorWidth = 480;
constexpr uint32_t kMonitorHeight = 270;
constexpr const char* kMonitorCameraName = "FeatureCheckMonitor";
constexpr KCE::Vector3 kMonitorCameraPosition = { 0.0f, 2.0f, -9.0f };
constexpr KCE::Vector3 kMonitorCameraRotation = { 0.15f, 0.0f, 0.0f };

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

// 床の上面の高さ（キューブの半分の厚み = スケールの Y だけ中心より上）
constexpr float kFloorTopHeight = kFloorPosition.y + kFloorScale.y;
// 被写界深度。デバッグカメラの初期位置から比較用キューブの列までの距離にピントを合わせる
constexpr float kDofFocusDistance = 19.0f;
constexpr float kDofFocusRange = 10.0f;

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

KCE::TextOverlay* GetTextOverlay()
{
	if (!KCE::SequencerEditor::HasInstance())
	{
		return nullptr;
	}
	return KCE::SequencerEditor::GetInstance()->GetPlayer().GetBindingContext().GetTextOverlay();
}

// 歌詞と会話の見本（自作の文）
constexpr const char* kSampleLyric = "光の中で 君と歌おう";
constexpr const char* kSampleSpeaker = "プロデューサー";
constexpr const char* kSampleLine = "今日のステージ、最高だったよ！次も一緒にがんばろう。";

// 3D 空間の文字の見本（自作の文）。比較用キューブの上に浮かべる。
// シーケンサの Text3D トラックの Target にこの名前を入れると動かせる
constexpr const char* kStageTextName = "FeatureCheck_StageText";
constexpr const char* kStageText = "ひかりのステージへ";
constexpr KCE::Vector3 kStageTextPosition = { 0.0f, 3.4f, 1.0f };
constexpr float kStageTextSize = 0.9f;
// 1 を超える色でブルームに乗せて光らせる
constexpr KCE::Vector4 kStageTextColor = { 2.5f, 1.6f, 0.5f, 1.0f };
// 見本の出入り。1秒に何文字出すか、出し切ってから止めておく時間、抜けた後の間
constexpr float kStageTextStep = 1.0f / 60.0f;
constexpr float kStageTextCharsPerSecond = 6.0f;
constexpr float kStageTextHoldSeconds = 1.5f;
constexpr float kStageTextGapSeconds = 0.5f;
// 周ごとに Fade → Drop → Spin → Pop と動き方を変える
constexpr int kStageTextStyleCount = 4;

void HideTextSample()
{
	if (auto* overlay = GetTextOverlay())
	{
		overlay->HideLyric();
		overlay->HideDialogue();
	}
}
} // namespace

void FeatureCheckScene::Initialize()
{
#ifdef USE_IMGUI
	KCE::DebugUIManager::GetInstance()->RegisterHierarchySection(this, "機能チェック", [this]() { this->DrawImGui(); });
#endif

	// 床
	floor_ = CreateObject("FeatureCheck_Floor", kFloorPosition, kFloorScale);
	floor_->SetColor(kFloorColor);

	// トゥーン・リムの比較。0 / 0.5 / 1 の3段階で並べる
	for (size_t i = 0; i < kShadingCubeCount; ++i)
	{
		const float offset = (static_cast<float>(i) - static_cast<float>(kShadingCubeCount - 1) * 0.5f) * kShadingCubeSpacing;
		const std::string name = "FeatureCheck_Shading" + std::to_string(i);
		shadingCubes_[i] = CreateObject(name, { offset, kShadingCubeHeight, kShadingCubeZ }, kShadingCubeScale);

		const float amount = static_cast<float>(i) / static_cast<float>(kShadingCubeCount - 1);
		if (auto* renderable = shadingCubes_[i]->GetRenderable3d())
		{
			renderable->SetColor(kShadingCubeColor);
			renderable->SetToonAmount(amount);
			renderable->SetRimStrength(amount);
			// 輪郭線は素材ごとの強さが 0 より大きいものにだけ引かれる（既定は 0）
			renderable->SetOutlineStrength(kShadingCubeOutlineStrength);
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
			// ライトが当たると赤と青が薄まって重なりが分かりにくいので、色をそのまま出す
			renderable->SetEnableLighting(false);
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

	// モニター。ライトを切って映像の色をそのまま出す
	monitorScreen_ = CreateObject("FeatureCheck_Monitor", kMonitorScreenPosition, kMonitorScreenScale);
	// モデルを差し替えると素材の設定も作り直されるので、色などより先に替える
	monitorScreen_->SetModel(kMonitorScreenModel);
	monitorScreen_->SetRotation(kMonitorScreenRotation);
	if (auto* renderable = monitorScreen_->GetRenderable3d())
	{
		renderable->SetRenderingType(KCE::RenderingType::Forward);
		renderable->SetEnableLighting(false);
		renderable->SetColor(kMonitorScreenColor);
	}
	stageMonitor_ = std::make_unique<KCE::StageMonitor>();
	if (stageMonitor_->Initialize(sceneManager_->GetSubViewProvider(), sceneManager_->GetCameraManager(), kMonitorCameraName, kMonitorWidth, kMonitorHeight))
	{
		stageMonitor_->GetCamera()->SetTranslate(kMonitorCameraPosition);
		stageMonitor_->GetCamera()->SetRotate(kMonitorCameraRotation);
		stageMonitor_->SetScreen(monitorScreen_.get());
	}

	SetupStageLights();
	SetupScreenQuality();

	// 3D 空間の文字。Text3DRenderer には非所有で登録する（外すのは OnFinalize）
	stageText_ = std::make_unique<KCE::TextMesh3D>();
	stageText_->SetText(kStageText);
	stageText_->GetParams().position = kStageTextPosition;
	stageText_->GetParams().size = kStageTextSize;
	stageText_->GetParams().color = kStageTextColor;
	if (auto* text3D = sceneManager_->GetText3D())
	{
		text3D->Register(kStageTextName, stageText_.get());
	}
	stageManager_ = std::make_unique<KCE::StageManager>();
	stageManager_->Initialize("feature_check", sceneManager_->GetText3D(), sceneManager_->GetCameraManager(),
		sceneManager_->GetSubViewProvider(), sceneManager_->GetObject3dCommon(), sceneManager_->GetLightManager());
	if (KCE::SequencerEditor::HasInstance())
	{
		KCE::SequencerEditor::GetInstance()->SetStageSaveCallback([this]() { return stageManager_ && stageManager_->SaveToFile(); });
	}

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
		// 霧は画面全体を白く霞ませて、他の項目の色や陰影が見えにくくなる。
		// 最初はオフにして、確かめるときだけパネルのチェックで入れる
		prevFogEnabled_ = fog->GetSettings().enabled;
		fog->GetSettings().enabled = false;
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

void FeatureCheckScene::SetupScreenQuality()
{
	// 床を反射させる。床そのものは反射の絵に描かないレイヤーへ移す
	floor_->SetRenderLayer(KCE::PlanarReflection::kReflectorLayer);
	if (auto* reflection = sceneManager_->GetPlanarReflection())
	{
		prevReflection_ = reflection->GetSettings();
		reflection->GetSettings().enabled = true;
		reflection->GetSettings().planeHeight = kFloorTopHeight;
	}
	// 光の筋はビームを出しているライト（ここではステージのスポット3本）に出る
	if (auto* volumetric = sceneManager_->GetVolumetricLight())
	{
		prevVolumetric_ = volumetric->GetSettings();
		volumetric->GetSettings().enabled = true;
	}
	if (auto* depthOfField = sceneManager_->GetDepthOfField())
	{
		prevDepthOfField_ = depthOfField->GetSettings();
		depthOfField->GetSettings().enabled = true;
		depthOfField->GetSettings().focusDistance = kDofFocusDistance;
		depthOfField->GetSettings().focusRange = kDofFocusRange;
	}
}

void FeatureCheckScene::RestoreAtmosphere()
{
	if (auto* reflection = sceneManager_->GetPlanarReflection())
	{
		reflection->GetSettings() = prevReflection_;
	}
	if (auto* volumetric = sceneManager_->GetVolumetricLight())
	{
		volumetric->GetSettings() = prevVolumetric_;
	}
	if (auto* depthOfField = sceneManager_->GetDepthOfField())
	{
		depthOfField->GetSettings() = prevDepthOfField_;
	}

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

	// 見本の文字を他のシーンに残さない
	HideTextSample();
	if (KCE::SequencerEditor::HasInstance())
	{
		KCE::SequencerEditor::GetInstance()->SetStageSaveCallback({});
	}
	// ステージの文字を先に登録解除してから、コード直書きの見本を片付ける
	stageManager_.reset();
	if (auto* text3D = sceneManager_->GetText3D())
	{
		text3D->Unregister(stageText_.get());
	}
	stageText_.reset();

	// 画面より先にモニターを畳む（画面のテクスチャを戻してからサブビューを消す）
	stageMonitor_.reset();

	// このシーンで足したスポットライトとその影を外す。残すと他のシーンでも当たり続ける
	for (const char* name : kSpotLightNames)
	{
		if (auto* lightManager = sceneManager_->GetLightManager())
		{
			lightManager->RemoveSpotLight(name);
		}
		if (auto* shadowMapManager = sceneManager_->GetShadowMapManager())
		{
			shadowMapManager->RemoveSpotLightShadowMap(name);
		}
	}

	// 破棄の前に登録を外す（GameObjectManager に死んだポインタを残さない）
	auto* manager = KCE::GameObjectManager::GetInstance();
	for (auto* object : { floor_.get(), transparentFront_.get(), transparentBack_.get(), hdrCube_.get(), monitorScreen_.get() })
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
	monitorScreen_.reset();
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

	if (stageMonitor_)
	{
		stageMonitor_->Update();
	}
	if (stageManager_)
	{
		stageManager_->Update();
	}

	if (showTextSample_)
	{
		if (auto* overlay = GetTextOverlay())
		{
			overlay->ShowLyric(kSampleLyric, 1.0f);
			overlay->ShowDialogue(kSampleSpeaker, kSampleLine, KCE::TextSprite::kShowAll, 1.0f);
		}
	}

	// 3D 文字の見本を、1文字ずつ出して・止めて・抜けさせる。周ごとに動き方を変える
	if (stageText_ && animateStageText_)
	{
		stageTextTime_ += kStageTextStep;
		const float charCount = static_cast<float>(stageText_->GetCharCount());
		const float revealSeconds = charCount / kStageTextCharsPerSecond;
		const float cycleSeconds = revealSeconds * 2.0f + kStageTextHoldSeconds + kStageTextGapSeconds;
		const float local = std::fmod(stageTextTime_, cycleSeconds);
		const int loop = static_cast<int>(stageTextTime_ / cycleSeconds);

		KCE::TextMesh3D::Params& params = stageText_->GetParams();
		params.style = static_cast<KCE::TextAppearStyle>(loop % kStageTextStyleCount);
		params.reveal = local * kStageTextCharsPerSecond;
		params.exit = (local - revealSeconds - kStageTextHoldSeconds) * kStageTextCharsPerSecond;
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

	if (ImGui::Checkbox("デバッグカメラ", &useDebugCamera_) && debugCamera_)
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

	ImGui::SeparatorText("半透明");
	if (ImGui::SliderFloat("不透明度", &transparentAlpha_, 0.0f, 1.0f))
	{
		ApplyTransparentColors();
	}

	ImGui::SeparatorText("大気");
	if (auto* fog = GetFogRenderer())
	{
		ImGui::Checkbox("フォグ", &fog->GetSettings().enabled);
	}
	if (auto* beam = GetBeamRenderer())
	{
		ImGui::Checkbox("ビーム", &beam->GetSettings().enabled);
	}

	ImGui::SeparatorText("文字");
	if (ImGui::Checkbox("文字のサンプル", &showTextSample_) && !showTextSample_)
	{
		HideTextSample();
	}
	ImGui::TextDisabled("シーケンサの Text トラックを試すときはオフにする。");
	ImGui::Checkbox("3D Text Demo", &animateStageText_);
	ImGui::TextDisabled("Text3D トラックで動かすときはオフにして、Target に FeatureCheck_StageText を入れる。");
	ImGui::TextDisabled("細かい値は Fog / Beam / Outline / PostProcess の各デバッグUIで調整する。");

	ImGui::SeparatorText("オブジェクト");
	for (size_t i = 0; i < kShadingCubeCount; ++i)
	{
		if (shadingCubes_[i])
		{
			ImGui::BulletText("%s", shadingCubes_[i]->GetName().c_str());
		}
	}
	for (auto* object : { floor_.get(), transparentFront_.get(), transparentBack_.get(), hdrCube_.get(), monitorScreen_.get() })
	{
		if (object)
		{
			ImGui::BulletText("%s", object->GetName().c_str());
		}
	}
#endif
}
