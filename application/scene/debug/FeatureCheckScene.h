#pragma once
#include <array>
#include <memory>
#include <string>

#include "camerawork/debug/DebugCamera.h"
#include "gameobject/base/GameObject.h"
#include "graphics/atmosphere/VolumetricLightRenderer.h"
#include "graphics/postfx/DepthOfFieldRenderer.h"
#include "graphics/text/TextMesh3D.h"
#include "graphics/view/PlanarReflection.h"
#include "graphics/view/StageMonitor.h"
#include "scene/interface/BaseScene.h"
#include "stage/StageManager.h"

/**
 * @brief 描画とシーケンサの新機能を1画面でまとめて目視確認するためのデバッグシーン。
 *
 * - 不透明 / 半透明 / トゥーン・リム・アウトライン / HDR / フォグ / ビームを並べておく
 * - キューブは GameObjectManager に登録するので、シーケンサの Preview Object で選べる
 * - ライトやフォグの設定はシーンを抜けるときに元へ戻す（他のシーンに残さない）
 */
class FeatureCheckScene : public KCE::BaseScene
{
public:
	void Initialize() override;
	void CommonUpdate() override;
	void Draw3D() override;
	void Draw2D() override;
	void DrawGBuffer() override;
	void DrawShadow() override;
	void DrawImGui() override;
	// ステージ（3D 文字・カメラ・モニター）を保存していなければ、シーンの切り替え前に確認を出す
	bool HasUnsavedChanges() const override { return stageManager_ && stageManager_->IsDirty(); }

protected:
	void OnFinalize() override;

private:
	/**
	 * @brief GameObject を作って GameObjectManager に登録する。
	 * @param name シーケンサの一覧に出る名前
	 * @param position 置く位置
	 * @param scale 大きさ
	 * @return 作ったオブジェクト。所有は受け取った側、GameObjectManager は非所有で持つ
	 */
	std::unique_ptr<KCE::GameObject> CreateObject(const std::string& name, const KCE::Vector3& position, const KCE::Vector3& scale);

	// スポットライトとビームを置く
	void SetupStageLights();

	// フォグ・ビーム・ライトを入る前の状態に戻す
	void RestoreAtmosphere();

	// 半透明キューブの色（アルファ）を今の設定で反映する
	void ApplyTransparentColors();

	// 床の反射・光の筋・被写界深度を入れる。元の設定は抜けるときに戻す
	void SetupScreenQuality();

	// 見た目の比較用に並べるキューブの数（トゥーン・リムを段階的に変える）
	static constexpr size_t kShadingCubeCount = 3;
	// ステージ用スポットライトの数
	static constexpr size_t kSpotLightCount = 3;

	// シーンが所有する GameObject。GameObjectManager には非所有で登録している
	std::unique_ptr<KCE::GameObject> floor_;
	std::array<std::unique_ptr<KCE::GameObject>, kShadingCubeCount> shadingCubes_;
	std::unique_ptr<KCE::GameObject> transparentFront_;
	std::unique_ptr<KCE::GameObject> transparentBack_;
	std::unique_ptr<KCE::GameObject> hdrCube_;
	// 奥に置く画面。StageMonitor の映像を映す
	std::unique_ptr<KCE::GameObject> monitorScreen_;
	std::unique_ptr<KCE::StageMonitor> stageMonitor_;
	// 3D 空間の文字の見本。Text3DRenderer には非所有で登録している
	std::unique_ptr<KCE::TextMesh3D> stageText_;
	// ステージ由来の物を所有し、Text3DRenderer より先に登録解除する
	std::unique_ptr<KCE::StageManager> stageManager_;
	// 見本の出入りを回すための経過（フレーム数ベース。デバッグ用なので実時間には合わせない）
	float stageTextTime_ = 0.0f;
	// 見本を自動で出入りさせるか。シーケンサの Text3D トラックを試すときはオフにする
	bool animateStageText_ = true;

	std::unique_ptr<KCE::DebugCamera> debugCamera_;
	// シーケンサのカメラ再生と取り合わないように、オフにできるようにしておく
	bool useDebugCamera_ = true;

	// 半透明キューブのアルファ
	float transparentAlpha_ = 0.5f;

	// 歌詞と会話の見本を出すか。シーケンサの Text トラックを試すときはオフにする
	bool showTextSample_ = true;

	// 入る前の状態（抜けるときに戻す）
	bool prevFogEnabled_ = false;
	bool prevBeamEnabled_ = false;
	KCE::PlanarReflection::Settings prevReflection_{};
	KCE::VolumetricLightRenderer::Settings prevVolumetric_{};
	KCE::DepthOfFieldRenderer::Settings prevDepthOfField_{};
};
