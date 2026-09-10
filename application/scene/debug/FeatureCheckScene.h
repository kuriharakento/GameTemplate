#pragma once
#include <array>
#include <memory>
#include <string>

#include "camerawork/debug/DebugCamera.h"
#include "gameobject/base/GameObject.h"
#include "scene/interface/BaseScene.h"

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

	std::unique_ptr<KCE::DebugCamera> debugCamera_;
	// シーケンサのカメラ再生と取り合わないように、オフにできるようにしておく
	bool useDebugCamera_ = true;

	// 半透明キューブのアルファ
	float transparentAlpha_ = 0.5f;

	// 入る前の状態（抜けるときに戻す）
	bool prevFogEnabled_ = false;
	bool prevBeamEnabled_ = false;
};
