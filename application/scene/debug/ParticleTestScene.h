#pragma once
#include "camerawork/debug/DebugCamera.h"
#include "scene/interface/BaseScene.h"
#include "effects/particle/editor/ParticleEditor.h"
#include "effects/particle/ParticleTypes.h"
#include <string>
#include <array>

/**
 * @brief パーティクルテストシーン
 */
class ParticleTestScene : public KCE::BaseScene
{
public:
	void Initialize() override;
	void Draw3D() override;
	void DrawGBuffer() override;
	void Draw2D() override;

	/**
	 * @brief シーン全体の共通更新処理
	 */
	void CommonUpdate() override;
	void DrawImGui() override;

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
	std::unique_ptr<KCE::Object3d> selectiveBloomTestObject_;

	// ベンチマーク用
	int activePreset_ = 0;
	bool isBenchmarkRunning_ = false;
	float benchmarkTime_ = 0.0f;
	float benchmarkSamplingTime_ = 0.0f;
	KCE::SimulationMode benchmarkMode_ = KCE::SimulationMode::CPU;
	std::unique_ptr<KCE::ParticleEffect> benchmarkEffect_;
	KCE::ParticleEffect* benchmarkEffectPtr_ = nullptr;
	bool statsResetDone_ = false;
	uint32_t benchmarkSamplingFrames_ = 0;
	uint64_t benchmarkActiveCountSum_ = 0;
	uint32_t benchmarkActiveCountSamples_ = 0;
	bool originalDiagnosticsEnabled_ = false;
	KCE::ParticleEffect* jsonGpuTestEffect_ = nullptr;
	float jsonGpuTestTime_ = 0.0f;
	bool jsonGpuTestWritten_ = false;
	KCE::ParticleEffect* stabilityEffect_ = nullptr;
	bool stabilityRunning_ = false;
	bool stabilityDestroying_ = false;
	uint32_t stabilityIteration_ = 0;
	uint32_t stabilityRecoveryFrames_ = 0;
	uint32_t stabilityWarmupFrames_ = 0;
	uint32_t stabilityOwnedDescriptors_ = 0;
	uint32_t stabilityBaselineDescriptors_ = 0;
	uint32_t stabilityPeakDescriptors_ = 0;
	KCE::ParticleEffect* selectiveBloomMatrixEffect_ = nullptr;
	uint32_t selectiveBloomMatrixStage_ = 0;
	uint32_t selectiveBloomMatrixInitialPsoCount_ = 0;
	bool selectiveBloomMatrixRunning_ = false;
	bool selectiveBloomMatrixFailed_ = false;
	std::string selectiveBloomMatrixFailure_;
	std::array<double, 28> selectiveBloomMatrixMaskEnergy_{};
	std::array<uint8_t, 28> selectiveBloomMatrixMaskSamples_{};
	uint64_t selectiveBloomMatrixLastDiagnosticId_ = UINT64_MAX;
	bool selectiveBloomMatrixMaskFinite_ = true;

	void StartBenchmark(int preset, KCE::SimulationMode mode);
	void StopBenchmark();
	void UpdateSelectiveBloomMatrixTest();
};
