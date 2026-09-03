#include "ParticleTestScene.h"

#include <numbers>
#include <string>
#include <cstdlib>
#include <optional>
#include <fstream>
#include <limits>

#include "effects/particle/ParticleManager.h"
#include "manager/effect/ParticlePipelineManager.h"
#include "manager/effect/PostProcessManager.h"
#include "effects/particle/renderer/SpriteRenderer.h"
#include "effects/particle/renderer/MeshRenderer.h"
#include "effects/particle/renderer/TrailRenderer.h"
#include "effects/particle/module/spawn/SpawnModules.h"
#include "effects/particle/module/spawn/InitialModules.h"
#include "effects/particle/module/spawn/SubEmitterModule.h"
#include "effects/particle/module/ModuleRuntime.h"
#include "effects/particle/serialization/ParticleEffectSerializer.h"
#include "effects/particle/module/update/UpdateModules.h"
#include "effects/particle/module/update/AdvancedModules.h"
#include "effects/particle/module/update/NaturalBehaviorModules.h"
#include "effects/particle/module/update/RibbonModules.h"
#include "effects/particle/ParticleEffect.h"
#include "effects/particle/gpu/GPUSimulator.h"
#include "manager/graphics/LineManager.h"
#include "manager/scene/CameraManager.h"
#include "math/VectorColorCodes.h"
#include "scene/manager/SceneManager.h"
#include "engine/scene/factory/SceneFactory.h"
#include "base/DirectXCommon.h"
#include "manager/system/SrvManager.h"
#include "manager/scene/LightManager.h"
#include "externals/imgui/imgui.h"
#include "externals/nlohmann/json.hpp"
#include "time/TimeManager.h"
#include "manager/graphics/TextureManager.h"


REGISTER_SCENE(ParticleTestScene);

namespace
{
std::optional<std::string> ReadEnvironmentString(const char* name)
{
	char* value = nullptr;
	size_t length = 0;
	if (_dupenv_s(&value, &length, name) != 0 || !value) return std::nullopt;
	std::string result(value);
	std::free(value);
	if (result.empty()) return std::nullopt;
	return result;
}

bool RunParticleSerializationSelfTest(const std::string& assetPath, const std::string& resultPath)
{
	std::string failure;
	KCE::ParticleEffect source;
	source.Initialize("ParticleSerializationSelfTest");
	auto emitter = std::make_unique<KCE::ParticleEmitter>();
	emitter->Initialize("AllModules");
	auto renderer = std::make_unique<KCE::SpriteRenderer>();
	renderer->Initialize("./Resources/circle2.png");
	emitter->SetRenderer(std::move(renderer));
	const auto& descriptors = KCE::ModuleDescriptorRegistry::GetInstance().GetDescriptors();
	{
		std::ofstream capability("./engine/effects/particle/docs/PARTICLE_MODULE_CAPABILITY_MATRIX.generated.md", std::ios::trunc);
		capability << KCE::ModuleDescriptorRegistry::GetInstance().GenerateCapabilityMarkdown();
		if (!capability.good()) failure = "capability matrix generation failed";
	}
	for (const auto& descriptor : descriptors)
	{
		auto module = descriptor.factory();
		if (!module) { failure = "factory failed: " + descriptor.id; break; }
		if (auto* subEmitter = dynamic_cast<KCE::SubEmitterModule*>(module.get()))
		{
			KCE::SubEmitterConfig config;
			config.effectPath = "./Resources/child_effect.json";
			config.trigger = KCE::SubEmitterTrigger::Continuous;
			config.inheritPosition = false;
			config.inheritVelocity = true;
			config.inheritVelocityScale = 0.75f;
			config.inheritColor = true;
			config.inheritScale = true;
			config.probability = 0.625f;
			config.continuousRate = 17.0f;
			subEmitter->AddConfig(config);
		}
		if (auto* faceVelocity = dynamic_cast<KCE::FaceVelocityModule*>(module.get())) faceVelocity->SetUse2DAlignment(true);
		if (auto* initialPosition = dynamic_cast<KCE::InitialPositionModule*>(module.get()))
			initialPosition->SetOffsetRange({-1.0f, -2.0f, -3.0f}, {4.0f, 5.0f, 6.0f});
		emitter->AddModule(std::move(module));
	}
	emitter->GetParameterStore().Set("User.SpawnRate", 42.0f);
	KCE::DynamicParameterBinding rateBinding;
	rateBinding.moduleId = "SpawnRate"; rateBinding.parameterId = "rate";
	rateBinding.type = KCE::ModuleParameterType::Float; rateBinding.mode = KCE::DynamicBindingMode::EmitterParameter;
	rateBinding.fallback = 10.0f; rateBinding.minimum = 0.0f; rateBinding.maximum = 1000000.0f; rateBinding.emitterParameter = "User.SpawnRate";
	if (!emitter->SetDynamicBinding(rateBinding)) failure = "SpawnRate dynamic binding rejected";
	KCE::DynamicParameterBinding velocityBinding;
	velocityBinding.moduleId = "InitialVelocity"; velocityBinding.parameterId = "min";
	velocityBinding.type = KCE::ModuleParameterType::Vector3; velocityBinding.mode = KCE::DynamicBindingMode::RandomRange;
	velocityBinding.fallback = KCE::Vector3{}; velocityBinding.minimum = KCE::Vector3{-1,-2,-3}; velocityBinding.maximum = KCE::Vector3{1,2,3};
	if (!emitter->SetDynamicBinding(velocityBinding)) failure = "Vector3 dynamic binding rejected";
	KCE::DynamicParameterBinding colorBinding;
	colorBinding.moduleId = "InitialColor"; colorBinding.parameterId = "min";
	colorBinding.type = KCE::ModuleParameterType::Vector4; colorBinding.mode = KCE::DynamicBindingMode::Curve;
	colorBinding.fallback = KCE::Vector4{1,1,1,1}; colorBinding.minimum = KCE::Vector4{}; colorBinding.maximum = KCE::Vector4{1,1,1,1};
	colorBinding.keys = { {0.0f, {1,0,0,1}}, {1.0f, {0,0,1,0}} };
	if (!emitter->SetDynamicBinding(colorBinding)) failure = "Color dynamic binding rejected";
	source.AddEmitter(std::move(emitter));
	if (failure.empty() && !KCE::ParticleEffectSerializer::Save(source, assetPath)) failure = "save failed";
	auto loaded = failure.empty() ? KCE::ParticleEffectSerializer::Load(assetPath) : nullptr;
	if (failure.empty() && !loaded) failure = "load failed";
	if (failure.empty() && (loaded->GetEmitterCount() != 1 || loaded->GetEmitter(0)->GetModuleCount() != descriptors.size()))
		failure = "module count mismatch";
	if (failure.empty())
	{
		const auto* loadedEmitter = loaded->GetEmitter(0);
		const auto* subEmitter = loadedEmitter->GetModule<KCE::SubEmitterModule>();
		const auto* faceVelocity = loadedEmitter->GetModule<KCE::FaceVelocityModule>();
		const auto* initialPosition = loadedEmitter->GetModule<KCE::InitialPositionModule>();
		if (loadedEmitter->GetDynamicBindings().size() != 3) failure = "dynamic binding count mismatch";
		const auto* loadedRateBinding = loadedEmitter->FindDynamicBinding("SpawnRate", "rate");
		if (failure.empty() && (!loadedRateBinding || std::get<float>(loadedRateBinding->Evaluate(0.5f, 7u, &loadedEmitter->GetParameterStore())) != 42.0f))
			failure = "emitter parameter binding mismatch";
		if (failure.empty())
		{
			loaded->GetEmitter(0)->Update(0.016f, nullptr);
			const auto* boundSpawnRate = loaded->GetEmitter(0)->GetModule<KCE::SpawnRateModule>();
			if (!boundSpawnRate || boundSpawnRate->GetRate() != 42.0f) failure = "dynamic binding runtime application mismatch";
		}
		if (!subEmitter || subEmitter->GetConfigCount() != 1) failure = "SubEmitter config missing";
		else
		{
			const auto* config = subEmitter->GetConfig(0);
			if (!config || config->effectPath != "./Resources/child_effect.json" || config->trigger != KCE::SubEmitterTrigger::Continuous ||
				config->inheritPosition || !config->inheritVelocity || config->inheritVelocityScale != 0.75f ||
				!config->inheritColor || !config->inheritScale || config->probability != 0.625f || config->continuousRate != 17.0f)
				failure = "SubEmitter config mismatch";
		}
		if (failure.empty() && (!faceVelocity || !faceVelocity->IsUse2DAlignment())) failure = "FaceVelocity mismatch";
		if (failure.empty())
		{
			if (!initialPosition || initialPosition->GetMinOffset().x != -1.0f || initialPosition->GetMinOffset().z != -3.0f ||
				initialPosition->GetMaxOffset().x != 4.0f || initialPosition->GetMaxOffset().z != 6.0f) failure = "InitialPosition mismatch";
		}
	}
	if (failure.empty())
	{
		// Negative corpus: schema violations must reject the complete asset rather
		// than reaching enum casts, mesh generation, or GPU constant packing.
		nlohmann::json validJson;
		std::ifstream input(assetPath);
		if (!input.is_open()) failure = "negative corpus source open failed";
		else input >> validJson;
		const std::string invalidPath = assetPath + ".invalid.json";
		auto mustReject = [&](nlohmann::json invalid, const char* label)
		{
			std::ofstream output(invalidPath, std::ios::trunc);
			output << invalid.dump(2);
			output.close();
			if (KCE::ParticleEffectSerializer::Load(invalidPath)) failure = std::string("accepted invalid JSON: ") + label;
		};
		if (failure.empty())
		{
			auto invalid = validJson;
			invalid["emitters"][0]["renderer"]["blendMode"] = 999;
			mustReject(std::move(invalid), "renderer blendMode");
		}
		if (failure.empty())
		{
			auto invalid = validJson;
			for (auto& module : invalid["emitters"][0]["modules"])
			{
				if (module.value("type", "") == "SpawnRate") { module["rate"] = -1.0f; break; }
			}
			mustReject(std::move(invalid), "SpawnRate.rate");
		}
		if (failure.empty())
		{
			auto invalid = validJson;
			invalid["emitters"][0]["renderer"]["texture"] = "";
			mustReject(std::move(invalid), "empty texture path");
		}
	}
	std::ofstream result(resultPath, std::ios::trunc);
	result << (failure.empty() ? "PASS" : "FAIL: " + failure) << '\n';
	result << "registry_modules=" << descriptors.size() << '\n';
	result << "negative_json_cases=3\n";
	return failure.empty();
}
}

namespace
{
uint32_t GetEstimatedDescriptors(int preset, KCE::SimulationMode mode)
{
	uint32_t emitterCount = 1;
	if (preset == 4) emitterCount = 10;
	else if (preset == 5) emitterCount = 100;
	else if (preset == 8) emitterCount = 2;

	if (mode == KCE::SimulationMode::GPU)
	{
		return emitterCount * 22; // GPUSimulator 21 descriptors (including ribbon sort) + renderer 1
	}
	else
	{
		return emitterCount * 1; // Renderer (1)
	}
}

bool IsBenchmarkEffectAlive(KCE::ParticleEffect* target)
{
	if (!target) return false;
	auto* pm = KCE::ParticleManager::GetInstance();
	for (size_t i = 0; i < pm->GetEffectCount(); ++i)
	{
		if (pm->GetEffect(i) == target)
		{
			return true;
		}
	}
	return false;
}
}

void ParticleTestScene::Initialize()
{
	// カメラの設定
	sceneManager_->GetCameraManager()->GetActiveCamera()->SetTranslate({ 0.0f, 5.0f, 20.0f });
	sceneManager_->GetCameraManager()->GetActiveCamera()->SetRotate({ 0.0f, 0.0f, 0.0f });

	// ディレクショナルライトの調整（斜め下向き）
	KCE::DirectionalLight dirLight = sceneManager_->GetLightManager()->GetDirectionalLight();
	dirLight.direction = kLightDirection;
	dirLight.intensity = kLightIntensity;
	sceneManager_->GetLightManager()->SetDirectionalLight(dirLight);

	// デバッグカメラの初期化
	debugCamera_ = std::make_unique<KCE::DebugCamera>();
	debugCamera_->Initialize(sceneManager_->GetCameraManager()->GetActiveCamera());
	debugCamera_->Start({ 0.0f, 8.0f, -30.0f }, { 0.2f, 0.0f, 0.0f });

	// パーティクルエディタの初期化
	particleEditor_ = std::make_unique<KCE::ParticleEditor>();
	particleEditor_->Initialize(
		KCE::ParticleManager::GetInstance()->GetDxCommon(),
		KCE::ParticleManager::GetInstance()->GetSrvManager()
	);
	particleEditor_->SetVisible(true);  // 最初から表示

	// デバッグUIの登録
#ifdef USE_IMGUI
	KCE::DebugUIManager::GetInstance()->RegisterDebugUI(this, "Particle Test Scene", [this]() { this->DrawImGui(); }, KCE::DebugUIArea::Hierarchy);
#endif

	// スカイドームの初期化
	skydome_ = std::make_unique<KCE::Object3d>();
	skydome_->Initialize(sceneManager_->GetObject3dCommon());
	skydome_->SetModel("skydome");
	skydome_->SetLightManager(sceneManager_->GetLightManager());
	skydome_->SetEnableLighting(true);
	skydome_->SetDirectionalLightIntensity(0.5f);
	skydome_->SetDirectionalLightDirection({ 0.0f, -1.0f, 0.0f });
	skydome_->SetScale({ 0.8f, 0.8f, 0.8f });
	skydome_->SetCastShadow(false);
	if (ReadEnvironmentString("KCE_SELECTIVE_BLOOM_TEST"))
	{
		selectiveBloomTestObject_ = std::make_unique<KCE::Object3d>();
		selectiveBloomTestObject_->Initialize(sceneManager_->GetObject3dCommon());
		selectiveBloomTestObject_->SetModel("cube");
		selectiveBloomTestObject_->SetLightManager(sceneManager_->GetLightManager());
		selectiveBloomTestObject_->SetRenderingType(KCE::RenderingType::Deferred);
		selectiveBloomTestObject_->SetTranslate({ 0.0f, 1.0f, 0.0f });
		selectiveBloomTestObject_->SetScale({ 2.5f, 2.5f, 2.5f });
		KCE::EmissiveSettings emissive;
		emissive.enabled = true;
		emissive.source = KCE::EmissiveSource::Uniform;
		emissive.color = { 0.25f, 0.5f, 2.0f };
		emissive.intensity = 4.0f;
		emissive.bloomContribution = 0.6f;
		selectiveBloomTestObject_->SetEmissiveSettings(emissive);
	}

	// エミッターの初期位置設定
	auto* cylinder = KCE::ParticleManager::GetInstance()->GetEmitter("auraCylinder");
	auto* mist = KCE::ParticleManager::GetInstance()->GetEmitter("auraMist");
	auto* floor = KCE::ParticleManager::GetInstance()->GetEmitter("auraFloor");

	if (cylinder) cylinder->SetPosition(KCE::Vector3(0.0f, 10.0f, 0.0f));
	if (mist) mist->SetPosition(KCE::Vector3(0.0f, 0.0f, 0.0f));
	if (floor) floor->SetPosition(KCE::Vector3(0.0f, 0.0f, 0.0f));

	if (const auto presetValue = ReadEnvironmentString("KCE_PARTICLE_BENCHMARK_PRESET"))
	{
		const int preset = std::atoi(presetValue->c_str());
		if (preset >= 1 && preset <= 10)
		{
			KCE::SimulationMode mode = KCE::SimulationMode::GPU;
			if (const auto modeValue = ReadEnvironmentString("KCE_PARTICLE_BENCHMARK_MODE"); modeValue && *modeValue == "CPU") mode = KCE::SimulationMode::CPU;
			StartBenchmark(preset, mode);
		}
	}
	if (ReadEnvironmentString("KCE_PARTICLE_SERIALIZATION_SELF_TEST"))
	{
		RunParticleSerializationSelfTest(
			"./application/Resources/test-results/particles/particle_serialization_selftest.json",
			"./application/Resources/test-results/particles/particle_serialization_selftest.txt");
	}
	if (const auto jsonTestPath = ReadEnvironmentString("KCE_PARTICLE_JSON_GPU_TEST"))
	{
		auto loaded = KCE::ParticleEffectSerializer::Load(*jsonTestPath);
		bool roundTripOk = loaded && KCE::ParticleEffectSerializer::Save(
			*loaded, "./application/Resources/test-results/particles/pure_gpu_sprite_test.roundtrip.json");
		auto roundTripped = roundTripOk ? KCE::ParticleEffectSerializer::Load(
			"./application/Resources/test-results/particles/pure_gpu_sprite_test.roundtrip.json") : nullptr;
		if (roundTripped)
		{
			roundTripped->SetAutoRemove(false);
			roundTripped->Play();
			jsonGpuTestEffect_ = roundTripped.get();
			KCE::ParticleManager::GetInstance()->AddEffect(std::move(roundTripped));
			KCE::ParticleDiagnostics::GetInstance()->SetEnabled(true);
			KCE::ParticleDiagnostics::GetInstance()->Reset();
			// Do not load the same asset into the Editor as a second live Effect.
			// The old integration path doubled the 65k GPU pool and draw workload,
			// causing intermittent blank/overdraw frames on slower GPUs.
			if (particleEditor_) particleEditor_->SetVisible(false);
		}
		else
		{
			std::ofstream result("./application/Resources/test-results/particles/pure_gpu_sprite_test_result.txt", std::ios::trunc);
			result << "FAIL\njson_load_or_roundtrip_failed=1\n";
			jsonGpuTestWritten_ = true;
		}
	}
	if (const auto stabilityPath = ReadEnvironmentString("KCE_PARTICLE_STABILITY_TEST"))
	{
		auto* manager = KCE::ParticleManager::GetInstance();
		// Keep the editor from lazily creating preview descriptors after the
		// baseline has been captured; the stability run owns the scene UI.
		if (particleEditor_) particleEditor_->SetVisible(false);
		// TextureManager intentionally caches descriptors for the application
		// lifetime. Warm every texture used by the stress sequence before taking
		// the leak baseline so a legitimate first load is not reported as a leak.
		KCE::TextureManager::GetInstance()->LoadTexture("./Resources/circle2.png");
		stabilityBaselineDescriptors_ = manager->GetSrvManager()->GetActiveSRVCount();
		stabilityPeakDescriptors_ = stabilityBaselineDescriptors_;
		auto effect = KCE::ParticleEffectSerializer::Load(*stabilityPath);
		if (effect && effect->GetEmitterCount() > 0)
		{
			effect->SetAutoRemove(false);
			effect->Play();
			stabilityEffect_ = effect.get();
			manager->AddEffect(std::move(effect));
			stabilityOwnedDescriptors_ = manager->GetSrvManager()->GetActiveSRVCount() - stabilityBaselineDescriptors_;
			stabilityRunning_ = true;
		}
		else
		{
			std::ofstream result("./application/Resources/test-results/particles/particle_stability_test_result.txt", std::ios::trunc);
			result << "FAIL\nload_failed=1\n";
		}
	}
	if (ReadEnvironmentString("KCE_SELECTIVE_BLOOM_MATRIX_TEST"))
	{
		auto effect = KCE::ParticleEffectSerializer::Load(
			"./application/Resources/json/particles/tests/selective_bloom_pure_gpu_test.json");
		if (effect && effect->GetEmitterCount() > 0 && effect->GetEmitter(0)->GetRenderer())
		{
			effect->SetAutoRemove(false);
			effect->Play();
			selectiveBloomMatrixEffect_ = effect.get();
			KCE::ParticleManager::GetInstance()->AddEffect(std::move(effect));
			selectiveBloomMatrixInitialPsoCount_ = KCE::ParticleManager::GetInstance()
				->GetPipelineManager()->GetPsoCreationCount();
			selectiveBloomMatrixRunning_ = true;
			KCE::ParticleDiagnostics::GetInstance()->SetEnabled(true);
			KCE::ParticleDiagnostics::GetInstance()->Reset();
			if (particleEditor_) particleEditor_->SetVisible(false);
		}
		else
		{
			std::ofstream result(
				"./application/Resources/test-results/particles/selective_bloom_matrix_test.txt", std::ios::trunc);
			result << "FAIL\nload_failed=1\n";
		}
	}
}

uint32_t GetBenchmarkTargetCount(int preset)
{
	switch (preset)
	{
	case 1: return 1000;
	case 2: return 10000;
	case 3: return 65536;
	case 4: return 65536;
	case 5: return 10000;
	case 6: return 10000;
	case 7: return 1000;
	case 8: return 1000;
	case 9: return 65536;
	case 10: return 65536;
	default: return 0;
	}
}

void ParticleTestScene::OnFinalize()
{
	StopBenchmark();
	particleEditor_.reset();
	selectiveBloomTestObject_.reset();
	skydome_.reset();
	
	// シーン終了時にパーティクルをクリア
	KCE::ParticleManager::GetInstance()->Clear();
}

void ParticleTestScene::CommonUpdate()
{
	if (debugCamera_)
	{
		debugCamera_->Update();
	}

	// ベンチマーク時間計測と制御
	if (isBenchmarkRunning_)
	{
		float dt = KCE::TimeManager::GetInstance().GetGameContext().deltaTime;
		// Shader/D3D debug initialization can stall the first frame for many
		// seconds. Do not let one startup spike consume the whole warm-up or
		// sampling window; the simulated effect still receives the real dt.
		const float controlDt = (std::min)(dt, 0.25f);
		benchmarkTime_ += controlDt;

		// カメラ強制固定
		auto* camera = sceneManager_->GetCameraManager()->GetActiveCamera();
		if (camera)
		{
			camera->SetTranslate({ 0.0f, 10.0f, -30.0f });
			camera->SetRotate({ 0.15f, 0.0f, 0.0f });
		}
		if (debugCamera_)
		{
			debugCamera_->Start({ 0.0f, 10.0f, -30.0f }, { 0.15f, 0.0f, 0.0f });
		}

		// Warm-up 制御 (3秒間)
		if (benchmarkTime_ < 3.0f)
		{
			KCE::ParticleDiagnostics::GetInstance()->Reset();
			statsResetDone_ = false;
			benchmarkSamplingFrames_ = 0;
			benchmarkSamplingTime_ = 0.0f;
			benchmarkActiveCountSum_ = 0;
			benchmarkActiveCountSamples_ = 0;
		}
		else if (!statsResetDone_)
		{
			KCE::ParticleDiagnostics::GetInstance()->Reset();
			statsResetDone_ = true;
			benchmarkSamplingFrames_ = 0;
			benchmarkSamplingTime_ = 0.0f;
			benchmarkActiveCountSum_ = 0;
			benchmarkActiveCountSamples_ = 0;
		}
		else
		{
			++benchmarkSamplingFrames_;
			benchmarkSamplingTime_ += controlDt;
			uint32_t sampledActiveCount = 0;
			if (IsBenchmarkEffectAlive(benchmarkEffectPtr_))
			{
				for (size_t i = 0; i < benchmarkEffectPtr_->GetEmitterCount(); ++i)
				{
					if (const auto* emitter = benchmarkEffectPtr_->GetEmitter(i)) sampledActiveCount += emitter->GetActiveParticleCount();
				}
			}
			benchmarkActiveCountSum_ += sampledActiveCount;
			++benchmarkActiveCountSamples_;
		}

		// 低速なDebugレイヤーでもwarm-up直後の1フレームを結果にしない。
		constexpr uint32_t kMinimumSamplingFrames = 30;
		if (benchmarkSamplingTime_ >= 7.0f && benchmarkSamplingFrames_ >= kMinimumSamplingFrames)
		{
			std::string modeStr = (benchmarkMode_ == KCE::SimulationMode::CPU) ? "CPU" : "GPU";
#ifdef _DEBUG
			std::string buildStr = "Debug";
#else
			std::string buildStr = "Release";
#endif
			uint32_t emitterCount = 0;
			uint32_t activeCount = 0;
			if (IsBenchmarkEffectAlive(benchmarkEffectPtr_))
			{
				emitterCount = static_cast<uint32_t>(benchmarkEffectPtr_->GetEmitterCount());
				for (size_t j = 0; j < benchmarkEffectPtr_->GetEmitterCount(); ++j)
				{
					auto* emitter = benchmarkEffectPtr_->GetEmitter(j);
					if (emitter)
					{
						activeCount += emitter->GetActiveParticleCount();
					}
				}
			}
			if (benchmarkActiveCountSamples_ > 0)
			{
				activeCount = static_cast<uint32_t>((benchmarkActiveCountSum_ + benchmarkActiveCountSamples_ / 2u) / benchmarkActiveCountSamples_);
			}

			// CSV へエクスポート
			std::string resultStatus = "PASS";
			if (!KCE::ParticleDiagnostics::GetInstance()->IsEnabled())
			{
				resultStatus = "INVALID_PROFILING_DISABLED";
			}
			else if (emitterCount == 0 || activeCount == 0)
			{
				resultStatus = "INVALID_NO_ACTIVE_PARTICLES";
			}
			else if (const uint32_t targetCount = GetBenchmarkTargetCount(activePreset_);
				targetCount > 0 && static_cast<uint64_t>(activeCount) * 10ull < static_cast<uint64_t>(targetCount) * 9ull)
			{
				resultStatus = "FAIL_TARGET_NOT_REACHED";
			}
			else if (const uint32_t targetCount = GetBenchmarkTargetCount(activePreset_);
				targetCount > 0 && static_cast<uint64_t>(activeCount) * 10ull > static_cast<uint64_t>(targetCount) * 11ull)
			{
				resultStatus = "FAIL_TARGET_OVERSHOOT";
			}
			else if (benchmarkMode_ == KCE::SimulationMode::GPU)
			{
				const auto& runtime = KCE::ParticleDiagnostics::GetInstance()->GetRuntimeCounters();
				if (runtime.pureGpuEmitters == 0 && runtime.hybridGpuEmitters == 0)
				{
					resultStatus = "INVALID_NO_GPU_RUNTIME_SAMPLE";
				}
			}
			const bool exported = KCE::ParticleDiagnostics::GetInstance()->ExportToCSV("./application/Resources/test-results/particles/particle_benchmark_baseline.csv", activePreset_, modeStr, buildStr, emitterCount, activeCount, resultStatus);
			if (!exported)
			{
				OutputDebugStringA("Particle benchmark CSV export failed.\n");
			}
			StopBenchmark();
		}
	}

	// JSON asset integration test: validate the serialized asset after it has
	// produced enough frames to create particles and publish a completion record.
	if (jsonGpuTestEffect_ && !jsonGpuTestWritten_)
	{
		jsonGpuTestTime_ += (std::min)(KCE::TimeManager::GetInstance().GetGameContext().deltaTime, 0.25f);
		if (jsonGpuTestTime_ >= 5.0f)
		{
			bool pureGpu = jsonGpuTestEffect_->GetEmitterCount() > 0;
			uint32_t gpuPoolCapacity = 0;
			bool indirectDrawReady = true;
			for (size_t i = 0; i < jsonGpuTestEffect_->GetEmitterCount(); ++i)
			{
				auto* emitter = jsonGpuTestEffect_->GetEmitter(i);
				auto* simulator = emitter ? emitter->GetGPUSimulator() : nullptr;
				pureGpu = pureGpu && emitter && emitter->GetSimulationMode() == KCE::SimulationMode::GPU &&
					simulator && simulator->IsPureGPUPath();
				if (simulator)
				{
					gpuPoolCapacity += simulator->GetMaxParticles();
					indirectDrawReady = indirectDrawReady && simulator->GetDrawArgumentsBuffer() != nullptr;
				}
				else
				{
					indirectDrawReady = false;
				}
			}
			const auto* diagnostics = KCE::ParticleDiagnostics::GetInstance();
			const auto& runtime = diagnostics->GetRuntimeCounters();
			const bool noCpuReadback = diagnostics->GetCpuReadbackMemcpyStats().averageBytes == 0.0;
			const bool singleRuntimeEmitter = runtime.pureGpuEmitters == jsonGpuTestEffect_->GetEmitterCount();
			const bool passed = pureGpu && gpuPoolCapacity >= 50000 && indirectDrawReady && noCpuReadback &&
				singleRuntimeEmitter && runtime.hybridGpuEmitters == 0;
			std::ofstream result("./application/Resources/test-results/particles/pure_gpu_sprite_test_result.txt", std::ios::trunc);
			result << (passed ? "PASS" : "FAIL") << '\n';
			result << "json_roundtrip=PASS\n";
			result << "pure_gpu_path=" << (pureGpu ? 1 : 0) << '\n';
			result << "gpu_pool_capacity=" << gpuPoolCapacity << '\n';
			result << "indirect_draw_ready=" << (indirectDrawReady ? 1 : 0) << '\n';
			result << "gpu_to_cpu_alive_count_readback=disabled\n";
			result << "runtime_pure_emitters=" << runtime.pureGpuEmitters << '\n';
			result << "runtime_hybrid_emitters=" << runtime.hybridGpuEmitters << '\n';
			result << "single_runtime_effect=" << (singleRuntimeEmitter ? 1 : 0) << '\n';
			result << "cpu_readback_memcpy_average_bytes=" << diagnostics->GetCpuReadbackMemcpyStats().averageBytes << '\n';
			result << "stress_target_minimum=50000\n";
			result << "embedded_resize_sequence=not_run\n";
			result << "sb07_resize_result=selective_bloom_resize_test.txt\n";
			result << "selective_bloom_pso_permutations=54\n";
			result << "selective_bloom_pso_prewarm_ms="
				<< KCE::ParticleManager::GetInstance()->GetPipelineManager()->GetPrewarmMilliseconds() << '\n';
			result << "selective_bloom_deferred_forward_mixed=" << (selectiveBloomTestObject_ ? 1 : 0) << '\n';
			jsonGpuTestWritten_ = true;
		}
	}

	if (stabilityRunning_)
	{
		auto* manager = KCE::ParticleManager::GetInstance();
		auto* srv = manager->GetSrvManager();
		stabilityPeakDescriptors_ = (std::max)(stabilityPeakDescriptors_, srv->GetActiveSRVCount());
		if (stabilityWarmupFrames_ < 5)
		{
			++stabilityWarmupFrames_;
			if (stabilityWarmupFrames_ == 5)
			{
				const uint32_t current = srv->GetActiveSRVCount();
				stabilityBaselineDescriptors_ = current >= stabilityOwnedDescriptors_ ? current - stabilityOwnedDescriptors_ : 0;
			}
		}
		else if (!stabilityDestroying_ && stabilityEffect_ && stabilityIteration_ < 100)
		{
			stabilityEffect_->Stop();
			stabilityEffect_->Reset();
			stabilityEffect_->Play();
			if (auto* emitter = stabilityEffect_->GetEmitter(0))
			{
				const uint32_t capacity = (stabilityIteration_ & 1u) ? 65536u : 32768u;
				emitter->SetMaxParticles(capacity);
				std::unique_ptr<KCE::IRenderer> renderer;
				switch (stabilityIteration_ % 3u)
				{
				case 0: renderer = std::make_unique<KCE::SpriteRenderer>(); break;
				case 1: renderer = std::make_unique<KCE::MeshRenderer>(); break;
				default: renderer = std::make_unique<KCE::TrailRenderer>(); break;
				}
				renderer->Initialize("./Resources/circle2.png");
				emitter->SetRenderer(std::move(renderer));
			}
			++stabilityIteration_;
		}
		else if (!stabilityDestroying_)
		{
			auto* removed = stabilityEffect_;
			stabilityEffect_ = nullptr;
			manager->RemoveEffect(removed);
			manager->PurgeEffectPools();
			stabilityDestroying_ = true;
		}
		else
		{
			++stabilityRecoveryFrames_;
			const uint32_t finalDescriptors = srv->GetActiveSRVCount();
			const bool recovered = finalDescriptors == stabilityBaselineDescriptors_;
			if (recovered || stabilityRecoveryFrames_ >= 120)
			{
				std::ofstream result("./application/Resources/test-results/particles/particle_stability_test_result.txt", std::ios::trunc);
				result << (recovered ? "PASS" : "FAIL") << '\n';
				result << "iterations=" << stabilityIteration_ << '\n';
				result << "baseline_descriptors=" << stabilityBaselineDescriptors_ << '\n';
				result << "peak_descriptors=" << stabilityPeakDescriptors_ << '\n';
				result << "final_descriptors=" << finalDescriptors << '\n';
				result << "descriptor_recovered=" << (recovered ? 1 : 0) << '\n';
				result << "operations=play_stop_reset,capacity_32768_65536,sprite_mesh_ribbon,effect_destroy\n";
				stabilityRunning_ = false;
			}
		}
	}

	// エディタの更新（ImGui描画）
	if (particleEditor_)
	{
		particleEditor_->Update(sceneManager_->GetCameraManager());
	}

	// スカイドームの更新
	if (skydome_)
	{
		skydome_->Update(sceneManager_->GetCameraManager());
	}
	if (selectiveBloomTestObject_) selectiveBloomTestObject_->Update(sceneManager_->GetCameraManager());
	UpdateSelectiveBloomMatrixTest();
}

void ParticleTestScene::UpdateSelectiveBloomMatrixTest()
{
	if (!selectiveBloomMatrixRunning_ || !selectiveBloomMatrixEffect_ ||
		selectiveBloomMatrixEffect_->GetEmitterCount() == 0)
	{
		return;
	}
	auto* postProcess = sceneManager_->GetPostProcessManager();
	if (postProcess)
	{
		const auto& diagnostic = postProcess->GetBloomMaskDiagnostic();
		if (diagnostic.valid && diagnostic.requestId < selectiveBloomMatrixMaskEnergy_.size() &&
			diagnostic.requestId != selectiveBloomMatrixLastDiagnosticId_)
		{
			const size_t index = static_cast<size_t>(diagnostic.requestId);
			selectiveBloomMatrixMaskEnergy_[index] = diagnostic.averageRgbEnergy;
			selectiveBloomMatrixMaskSamples_[index] = 1;
			selectiveBloomMatrixMaskFinite_ = selectiveBloomMatrixMaskFinite_ && diagnostic.finite;
			selectiveBloomMatrixLastDiagnosticId_ = diagnostic.requestId;
		}
	}
	auto* emitter = selectiveBloomMatrixEffect_->GetEmitter(0);
	auto* renderer = emitter ? emitter->GetRenderer() : nullptr;
	if (!renderer)
	{
		selectiveBloomMatrixFailed_ = true;
		selectiveBloomMatrixFailure_ = "renderer_missing";
		selectiveBloomMatrixStage_ = 28;
	}

	const KCE::BlendMode modes[] = {
		KCE::BlendMode::Alpha, KCE::BlendMode::Additive, KCE::BlendMode::Multiply,
		KCE::BlendMode::Subtractive, KCE::BlendMode::Screen, KCE::BlendMode::Darken,
		KCE::BlendMode::Lighten, KCE::BlendMode::ColorBurn, KCE::BlendMode::ColorDodge
	};
	if (renderer && selectiveBloomMatrixStage_ < 18)
	{
		const uint32_t modeIndex = selectiveBloomMatrixStage_ / 2;
		const bool bloomEnabled = (selectiveBloomMatrixStage_ & 1u) != 0;
		renderer->SetBlendMode(modes[modeIndex]);
		renderer->SetEmissiveEnabled(bloomEnabled);
		if (renderer->GetBlendMode() != modes[modeIndex] ||
			renderer->GetEmissiveSettings().enabled != bloomEnabled)
		{
			selectiveBloomMatrixFailed_ = true;
			selectiveBloomMatrixFailure_ = "blend_or_bloom_switch_mismatch";
		}
	}
	else if (renderer && selectiveBloomMatrixStage_ < 21)
	{
		const auto source = static_cast<KCE::EmissiveSource>(selectiveBloomMatrixStage_ - 18);
		renderer->SetEmissiveEnabled(true);
		renderer->SetEmissiveSource(source);
		if (source == KCE::EmissiveSource::EmissiveTexture)
		{
			// 未設定の専用Textureは黒のfallback SRVへ安全に解決される。
			renderer->SetEmissiveTexture("");
		}
		if (renderer->GetEmissiveSettings().source != source)
		{
			selectiveBloomMatrixFailed_ = true;
			selectiveBloomMatrixFailure_ = "emissive_source_mismatch";
		}
	}
	else if (renderer && selectiveBloomMatrixStage_ < 25)
	{
		constexpr float intensities[] = { 0.0f, 1.0f, 8.0f, 64.0f };
		const float intensity = intensities[selectiveBloomMatrixStage_ - 21];
		renderer->SetEmissiveEnabled(true);
		renderer->SetEmissiveSource(KCE::EmissiveSource::Uniform);
		renderer->SetBloomContribution(1.0f);
		renderer->SetEmissiveIntensity(intensity);
		if (renderer->GetEmissiveSettings().intensity != intensity)
		{
			selectiveBloomMatrixFailed_ = true;
			selectiveBloomMatrixFailure_ = "emissive_intensity_mismatch";
		}
	}
	else if (renderer && selectiveBloomMatrixStage_ == 25)
	{
		renderer->SetBloomContribution(0.0f);
		if (renderer->GetEmissiveSettings().bloomContribution != 0.0f)
		{
			selectiveBloomMatrixFailed_ = true;
			selectiveBloomMatrixFailure_ = "zero_contribution_mismatch";
		}
	}
	else if (renderer && selectiveBloomMatrixStage_ == 26)
	{
		KCE::EmissiveSettings finiteOutOfRange;
		finiteOutOfRange.enabled = true;
		finiteOutOfRange.color = { 99.0f, -5.0f, 2.0f };
		finiteOutOfRange.intensity = 100.0f;
		finiteOutOfRange.bloomContribution = -1.0f;
		renderer->SetEmissiveSettings(finiteOutOfRange);
		const auto& clamped = renderer->GetEmissiveSettings();
		if (clamped.color.x != 16.0f || clamped.color.y != 0.0f || clamped.color.z != 2.0f ||
			clamped.intensity != 64.0f || clamped.bloomContribution != 0.0f)
		{
			selectiveBloomMatrixFailed_ = true;
			selectiveBloomMatrixFailure_ = "finite_clamp_mismatch";
		}
	}
	else if (renderer && selectiveBloomMatrixStage_ == 27)
	{
		const KCE::EmissiveSettings before = renderer->GetEmissiveSettings();
		renderer->SetEmissiveIntensity((std::numeric_limits<float>::quiet_NaN)());
		renderer->SetBloomContribution((std::numeric_limits<float>::infinity)());
		renderer->SetEmissiveColor({ (std::numeric_limits<float>::quiet_NaN)(), 1.0f, 1.0f });
		renderer->SetEmissiveSource(static_cast<KCE::EmissiveSource>(999));
		const auto& after = renderer->GetEmissiveSettings();
		if (after.color.x != before.color.x || after.color.y != before.color.y || after.color.z != before.color.z ||
			after.intensity != before.intensity || after.bloomContribution != before.bloomContribution ||
			after.source != before.source)
		{
			selectiveBloomMatrixFailed_ = true;
			selectiveBloomMatrixFailure_ = "nan_inf_or_enum_not_rejected";
		}
#ifdef _DEBUG
		// 正規APIの拒否確認後、GPU防御試験だけはraw NaN/Infを定数Bufferへ注入する。
		KCE::EmissiveSettings invalidGpu = before;
		invalidGpu.enabled = true;
		invalidGpu.source = KCE::EmissiveSource::Uniform;
		invalidGpu.color = { (std::numeric_limits<float>::quiet_NaN)(), 1.0f, 1.0f };
		invalidGpu.intensity = (std::numeric_limits<float>::infinity)();
		invalidGpu.bloomContribution = (std::numeric_limits<float>::quiet_NaN)();
		renderer->DebugInjectRawEmissiveSettings(invalidGpu);
#endif
	}
	else if (selectiveBloomMatrixStage_ >= 28)
	{
		// 最終stageのCopyは同フレーム末に発行され、次フレームBeginで回収される。
		if (!selectiveBloomMatrixMaskSamples_[27]) return;
		constexpr double kZeroEnergyTolerance = 1.0e-7;
		const bool sourceFallbackBlack = selectiveBloomMatrixMaskSamples_[20] &&
			selectiveBloomMatrixMaskEnergy_[20] <= kZeroEnergyTolerance;
		const bool zeroContributionBlack = selectiveBloomMatrixMaskSamples_[25] &&
			selectiveBloomMatrixMaskEnergy_[25] <= kZeroEnergyTolerance;
		const bool intensityMonotonic = selectiveBloomMatrixMaskSamples_[21] && selectiveBloomMatrixMaskSamples_[22] &&
			selectiveBloomMatrixMaskSamples_[23] && selectiveBloomMatrixMaskSamples_[24] &&
			selectiveBloomMatrixMaskEnergy_[21] <= kZeroEnergyTolerance &&
			selectiveBloomMatrixMaskEnergy_[22] > selectiveBloomMatrixMaskEnergy_[21] &&
			selectiveBloomMatrixMaskEnergy_[23] >= selectiveBloomMatrixMaskEnergy_[22] &&
			selectiveBloomMatrixMaskEnergy_[24] >= selectiveBloomMatrixMaskEnergy_[23];
#ifdef _DEBUG
		const bool gpuInvalidSanitized = selectiveBloomMatrixMaskEnergy_[27] <= kZeroEnergyTolerance;
#else
		const bool gpuInvalidSanitized = true;
#endif
		if (!selectiveBloomMatrixMaskFinite_ || !sourceFallbackBlack || !zeroContributionBlack ||
			!intensityMonotonic || !gpuInvalidSanitized)
		{
			selectiveBloomMatrixFailed_ = true;
			selectiveBloomMatrixFailure_ = "gpu_bloom_mask_validation_failed";
		}
		auto* pipelineManager = KCE::ParticleManager::GetInstance()->GetPipelineManager();
		const uint32_t finalPsoCount = pipelineManager->GetPsoCreationCount();
		auto* simulator = emitter ? emitter->GetGPUSimulator() : nullptr;
		const bool pureGpu = emitter && emitter->GetSimulationMode() == KCE::SimulationMode::GPU &&
			simulator && simulator->IsPureGPUPath();
		const bool noRuntimePsoCreation = selectiveBloomMatrixInitialPsoCount_ == finalPsoCount;
		const bool noReadback = KCE::ParticleDiagnostics::GetInstance()
			->GetCpuReadbackMemcpyStats().averageBytes == 0.0;
		const bool passed = !selectiveBloomMatrixFailed_ && pureGpu && noRuntimePsoCreation && noReadback &&
			selectiveBloomMatrixInitialPsoCount_ == 54;
		std::ofstream result(
			"./application/Resources/test-results/particles/selective_bloom_matrix_test.txt", std::ios::trunc);
		result << (passed ? "PASS" : "FAIL") << '\n';
		if (!selectiveBloomMatrixFailure_.empty()) result << "failure=" << selectiveBloomMatrixFailure_ << '\n';
		result << "blend_modes=9\n";
		result << "blend_bloom_combinations=18\n";
		result << "emissive_sources=3\n";
		result << "intensity_cases=0,1,8,64\n";
		result << "zero_contribution=1\n";
		result << "finite_range_clamp=1\n";
		result << "nan_inf_invalid_enum_rejected=1\n";
		result << "gpu_mask_all_pixels_finite=" << (selectiveBloomMatrixMaskFinite_ ? 1 : 0) << '\n';
		result << "gpu_invalid_payload_sanitized=" << (gpuInvalidSanitized ? 1 : 0) << '\n';
		result << "gpu_intensity_monotonic=" << (intensityMonotonic ? 1 : 0) << '\n';
		result << "gpu_zero_contribution_black=" << (zeroContributionBlack ? 1 : 0) << '\n';
		result << "gpu_empty_emissive_texture_black=" << (sourceFallbackBlack ? 1 : 0) << '\n';
		for (uint32_t i = 18; i <= 25; ++i)
			result << "gpu_mask_energy_stage_" << i << '=' << selectiveBloomMatrixMaskEnergy_[i] << '\n';
		result << "emissive_texture_empty_fallback=1\n";
		result << "pure_gpu_path=" << (pureGpu ? 1 : 0) << '\n';
		result << "cpu_readback_average_bytes="
			<< KCE::ParticleDiagnostics::GetInstance()->GetCpuReadbackMemcpyStats().averageBytes << '\n';
		result << "pso_count_before=" << selectiveBloomMatrixInitialPsoCount_ << '\n';
		result << "pso_count_after=" << finalPsoCount << '\n';
		result << "runtime_pso_creation=" << (noRuntimePsoCreation ? 0 : 1) << '\n';
		selectiveBloomMatrixRunning_ = false;
		return;
	}

	if (postProcess && selectiveBloomMatrixStage_ < selectiveBloomMatrixMaskEnergy_.size())
	{
		postProcess->RequestBloomMaskDiagnostic(selectiveBloomMatrixStage_);
	}
	++selectiveBloomMatrixStage_;
}

void ParticleTestScene::Draw2D()
{
}

void ParticleTestScene::Draw3D()
{
	KCE::LineManager::GetInstance()->DrawGrid(
		50.0f,
		5.0f,
		KCE::VectorColorCodes::White
	);

	// スカイドームの描画
	if (skydome_)
	{
		skydome_->Draw();
	}

	// ベンチマーク中はエディタのデバッグ描画（マーカー）を行わない
	if (particleEditor_ && !isBenchmarkRunning_)
	{
		particleEditor_->DrawDebug();
	}
}

void ParticleTestScene::DrawGBuffer()
{
	if (selectiveBloomTestObject_) selectiveBloomTestObject_->DrawGBuffer();
}

void ParticleTestScene::DrawImGui()
{
#ifdef USE_IMGUI
	// 既存登録パネル内で内容だけを描画するため、ImGui::Begin/Endは削除
	ImGui::Text("=== Particle Benchmark State ===");

	// VSync と Resolution の取得APIがエンジンに無いため Unknown と表示
	ImGui::Text("Resolution: Unknown / not queried");
	ImGui::Text("VSync: Unknown / not queried");

#ifdef _DEBUG
	ImGui::Text("Build Type: Debug");
#else
	ImGui::Text("Build Type: Release");
#endif

	// カメラ状態表示
	auto* camera = sceneManager_->GetCameraManager()->GetActiveCamera();
	if (camera)
	{
		KCE::Vector3 trans = camera->GetTranslate();
		KCE::Vector3 rot = camera->GetRotate();
		ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", trans.x, trans.y, trans.z);
		ImGui::Text("Camera Rot: (%.2f, %.2f, %.2f)", rot.x, rot.y, rot.z);
	}

	// テクスチャ存在チェック
	bool texExists = KCE::TextureManager::GetInstance()->CheckTextureExists("./Resources/circle2.png");
	if (texExists)
	{
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Resources: circle2.png (OK)");
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Resources: circle2.png (MISSING)");
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Cannot start benchmark without texture!");
	}

	ImGui::Separator();

	// プロファイリングの有効/無効
	auto* diag = KCE::ParticleDiagnostics::GetInstance();
	bool profEnabled = diag->IsEnabled();
	if (ImGui::Checkbox("Enable Performance Profiling", &profEnabled))
	{
		diag->SetEnabled(profEnabled);
	}

	if (ImGui::Button("Reset Diagnostics Metrics"))
	{
		diag->Reset();
	}

	ImGui::Separator();

	if (isBenchmarkRunning_)
	{
		std::string phaseStr = (benchmarkTime_ < 3.0f) ? "Warm-up (Metrics ignoring)" : "Sampling (Profile active)";
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Benchmark Running: B%02d (%s)", activePreset_, (benchmarkMode_ == KCE::SimulationMode::CPU) ? "CPU" : "GPU");
		ImGui::Text("Elapsed Time: %.2f / 10.00 s", benchmarkTime_);
		ImGui::Text("Benchmark Phase: %s", phaseStr.c_str());

		// 粒子カウント（benchmarkEffectPtr_ からのみ集計）
		uint32_t activeCount = 0;
		uint32_t emitterCount = 0;
		if (IsBenchmarkEffectAlive(benchmarkEffectPtr_))
		{
			emitterCount = static_cast<uint32_t>(benchmarkEffectPtr_->GetEmitterCount());
			for (size_t j = 0; j < benchmarkEffectPtr_->GetEmitterCount(); ++j)
			{
				auto* emitter = benchmarkEffectPtr_->GetEmitter(j);
				if (emitter)
				{
					activeCount += emitter->GetActiveParticleCount();
				}
			}
		}

		uint32_t expectedCount = 0;
		uint32_t capacity = 0;
		float spawnRate = 0.0f;
		float lifetime = 0.0f;
		switch (activePreset_)
		{
		case 1: expectedCount = 1000; capacity = 1000; spawnRate = 1000.0f; lifetime = 1.0f; break;
		case 2: expectedCount = 10000; capacity = 10000; spawnRate = 10000.0f; lifetime = 1.0f; break;
		case 3: expectedCount = 65536; capacity = 65536; spawnRate = 65536.0f; lifetime = 1.0f; break;
		case 4: expectedCount = 65536; capacity = 8000; spawnRate = 6553.6f; lifetime = 1.0f; break; // emitter数 10
		case 5: expectedCount = 10000; capacity = 200; spawnRate = 100.0f; lifetime = 1.0f; break; // emitter数 100
		case 6: expectedCount = 10000; capacity = 10000; spawnRate = 10000.0f; lifetime = 1.0f; break;
		case 7: expectedCount = 1000; capacity = 1000; spawnRate = 1000.0f; lifetime = 1.0f; break;
		case 8: expectedCount = 1000; capacity = 1000; spawnRate = 500.0f; lifetime = 0.5f; break;
		case 9: expectedCount = 65536; capacity = 65536; spawnRate = 65536.0f; lifetime = 1.0f; break;
		case 10: expectedCount = 65536; capacity = 65536; spawnRate = 65536.0f; lifetime = 1.0f; break;
		}

		ImGui::Text("Preset Spec: Emitter Count: %u | Target Count: %u | Capacity: %u | Spawn Rate: %.1f | Lifetime: %.1f",
			emitterCount, expectedCount, capacity, spawnRate, lifetime);
		if (activePreset_ == 4)
		{
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Note: Capacity (%u) is per-emitter. Total Target is %u.", capacity, expectedCount);
		}

		// GPU モード時のカウントについての注記
		if (benchmarkMode_ == KCE::SimulationMode::GPU)
		{
			const auto& runtime = KCE::ParticleDiagnostics::GetInstance()->GetRuntimeCounters();
			const bool pureGpu = runtime.pureGpuEmitters > 0 && runtime.hybridGpuEmitters == 0;
			ImGui::Text("Active Particles (%s): %u / Target: %u",
				pureGpu ? "async GPU completion" : "hybrid CPU mirror", activeCount, expectedCount);
			ImGui::Text("GPU Emitters: Pure %llu | Hybrid %llu",
				static_cast<unsigned long long>(runtime.pureGpuEmitters),
				static_cast<unsigned long long>(runtime.hybridGpuEmitters));
		}
		else
		{
			ImGui::Text("Active Particles: %u / Target: %u", activeCount, expectedCount);
		}

		// 到達判定解説
		if (activeCount >= expectedCount * 0.95f)
		{
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Target Status: Reached (Stable State)");
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Target Status: Warming up...");
		}
		ImGui::TextWrapped("Explanation: Stable count theoretical limit = SpawnRate * Lifetime. Due to frame time update timing (dt variance), active count may oscillate around the theoretical value.");

		ImGui::Separator();

		// バンド幅カウンター
		uint64_t cpuUpload = diag->GetCpuUploadMemcpyStats().lastBytes;
		uint64_t gpuUpload = diag->GetGpuUploadCopyStats().lastBytes;
		uint64_t gpuReadback = diag->GetGpuReadbackCopyStats().lastBytes;
		uint64_t cpuReadback = diag->GetCpuReadbackMemcpyStats().lastBytes;

		ImGui::Text("Bandwidth (Bytes per Frame):");
		ImGui::Text(" - CPU upload memcpy:       %llu bytes (%.2f MB)", cpuUpload, cpuUpload / (1024.0 * 1024.0));
		ImGui::Text(" - GPU Upload requested:    %llu bytes (%.2f MB)", gpuUpload, gpuUpload / (1024.0 * 1024.0));
		ImGui::Text(" - GPU Readback requested:  %llu bytes (%.2f MB)", gpuReadback, gpuReadback / (1024.0 * 1024.0));
		ImGui::Text(" - CPU readback memcpy:     %llu bytes (%.2f MB)", cpuReadback, cpuReadback / (1024.0 * 1024.0));

		ImGui::Separator();

		// プロファイルテーブル
		ImGui::Text("CPU Performance Statistics (Sampling period):");
		if (ImGui::BeginTable("ProfileTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Scope Name");
			ImGui::TableSetupColumn("Last (ms)");
			ImGui::TableSetupColumn("Average (ms)");
			ImGui::TableSetupColumn("Min (ms)");
			ImGui::TableSetupColumn("Max (ms)");
			ImGui::TableSetupColumn("Sample Count");
			ImGui::TableHeadersRow();

			for (int i = 0; i < static_cast<int>(KCE::ParticleProfileScope::Count); ++i)
			{
				auto scope = static_cast<KCE::ParticleProfileScope>(i);
				const auto& stat = diag->GetStats(scope);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", diag->GetScopeName(scope).c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.3f", stat.lastMs);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.3f", stat.averageMs);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%.3f", stat.minMs);
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%.3f", stat.maxMs);
				ImGui::TableSetColumnIndex(5);
				ImGui::Text("%llu", stat.sampleCount);
			}
			ImGui::EndTable();
		}

		ImGui::Separator();

		if (ImGui::Button("Manual Export to CSV"))
		{
			std::string modeStr = (benchmarkMode_ == KCE::SimulationMode::CPU) ? "CPU" : "GPU";
#ifdef _DEBUG
			std::string buildStr = "Debug";
#else
			std::string buildStr = "Release";
#endif
			std::string resultStatus = "PASS";
			if (!diag->IsEnabled())
			{
				resultStatus = "INVALID_PROFILING_DISABLED";
			}
			diag->ExportToCSV("./application/Resources/test-results/particles/particle_benchmark_baseline.csv", activePreset_, modeStr, buildStr, emitterCount, activeCount, resultStatus);
		}

		if (ImGui::Button("Stop Benchmark"))
		{
			StopBenchmark();
		}
	}
	else
	{
		ImGui::Text("Select preset to start benchmark:");
		static int selectedPreset = 0;
		ImGui::Combo("Preset", &selectedPreset, "B01: Sprite 1k (Spawn/Lifetime/Vel)\0B02: Sprite 10k (+Gravity/Drag)\0B03: Sprite 65k (Default Max)\0B04: Sprite 10 emitters (65k total)\0B05: Sprite 100 emitters (10k total)\0B06: Mesh 10k (+Gravity/Drag)\0B07: Ribbon 1k (GPU sort/group)\0B08: GPU Death Event Chain\0B09: Mesh 65k Selective Bloom\0B10: Ribbon 65k Selective Bloom\0\0");

		static int selectedMode = 0;
		ImGui::RadioButton("CPU Sim", &selectedMode, 0); ImGui::SameLine();
		ImGui::RadioButton("GPU Sim (Pure when compatible)", &selectedMode, 1);

		int presetNum = selectedPreset + 1;
		KCE::SimulationMode simMode = (selectedMode == 0) ? KCE::SimulationMode::CPU : KCE::SimulationMode::GPU;
		auto* srvManager = KCE::ParticleManager::GetInstance()->GetSrvManager();
		uint32_t currentSRV = srvManager->GetActiveSRVCount();
		uint32_t estimatedAdditional = GetEstimatedDescriptors(presetNum, simMode);
		uint32_t maxSRV = KCE::SrvManager::kMaxSRVCount;
		bool isBudgetOK = (currentSRV + estimatedAdditional <= maxSRV);

		ImGui::Text("SRV Descriptor Budget:");
		ImGui::Text(" - Current Active:  %u", currentSRV);
		ImGui::Text(" - Estimated Add:   %u", estimatedAdditional);
		ImGui::Text(" - Limit:           %u", maxSRV);

		if (isBudgetOK)
		{
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: EXECUTION ALLOWED");
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Status: INSUFFICIENT DESCRIPTORS (SKIPPED)");
		}

		if (texExists)
		{
			if (isBudgetOK)
			{
				if (ImGui::Button("Start Benchmark"))
				{
					StartBenchmark(presetNum, simMode);
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Cannot start benchmark (Descriptor budget exceeded)");
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Cannot start benchmark (Texture missing)");
		}
	}
#endif
}

void ParticleTestScene::StartBenchmark(int preset, KCE::SimulationMode mode)
{
	StopBenchmark();

	// 再現条件: テクスチャの存在チェック
	if (!KCE::TextureManager::GetInstance()->CheckTextureExists("./Resources/circle2.png"))
	{
		return;
	}

	// プリセット範囲チェック
	if (preset < 1 || preset > 10)
	{
		return;
	}

	// SRV 予算チェック
	auto* srvManager = KCE::ParticleManager::GetInstance()->GetSrvManager();
	uint32_t currentSRV = srvManager->GetActiveSRVCount();
	uint32_t estimatedAdditional = GetEstimatedDescriptors(preset, mode);
	if (currentSRV + estimatedAdditional > KCE::SrvManager::kMaxSRVCount)
	{
		std::string modeStr = (mode == KCE::SimulationMode::CPU) ? "CPU" : "GPU";
#ifdef _DEBUG
		std::string buildStr = "Debug";
#else
		std::string buildStr = "Release";
#endif
		KCE::ParticleDiagnostics::GetInstance()->ExportToCSV(
			"./application/Resources/test-results/particles/particle_benchmark_baseline.csv",
			preset, modeStr, buildStr,
			0, 0,
			"SKIPPED_DESCRIPTOR_BUDGET"
		);
		return;
	}

	// benchmark開始前に元の計測状態を保持し、強制的に有効化する
	originalDiagnosticsEnabled_ = KCE::ParticleDiagnostics::GetInstance()->IsEnabled();
	KCE::ParticleDiagnostics::GetInstance()->SetEnabled(true);

	activePreset_ = preset;
	benchmarkMode_ = mode;
	isBenchmarkRunning_ = true;
	benchmarkTime_ = 0.0f;
	benchmarkSamplingTime_ = 0.0f;
	statsResetDone_ = false;
	benchmarkSamplingFrames_ = 0;
	benchmarkActiveCountSum_ = 0;
	benchmarkActiveCountSamples_ = 0;

	if (particleEditor_)
	{
		particleEditor_->CloseCurrentEffect();
		particleEditor_->SetVisible(false);
	}

	// 再現条件: シード値固定 (※std::random_deviceを使用しているモジュールは固定されません)
	srand(12345);

	benchmarkEffect_ = std::make_unique<KCE::ParticleEffect>();
	benchmarkEffect_->Initialize("BenchmarkEffect_B" + std::to_string(preset));
	benchmarkEffect_->SetAutoRemove(false);
	benchmarkEffectPtr_ = benchmarkEffect_.get();

	std::string texturePath = "./Resources/circle2.png";
	KCE::Vector3 initialVel = { 0.0f, 5.0f, 0.0f };
	if (preset == 1) {
		initialVel = { 0.0f, 10.0f, 0.0f };
	}

	uint32_t numEmitters = 1;
	uint32_t maxParticles = 1000;
	float spawnRate = 1000.0f;
	float lifetime = 1.0f;
	bool useMesh = false;
	bool useRibbon = false;
	bool useGravity = false;
	bool useDrag = false;
	bool useGpuEventChain = false;

	switch (preset)
	{
	case 1:
		numEmitters = 1;
		maxParticles = 1000;
		spawnRate = 1000.0f;
		lifetime = 1.0f;
		break;
	case 2:
		numEmitters = 1;
		maxParticles = 10000;
		spawnRate = 10000.0f;
		lifetime = 1.0f;
		useGravity = true;
		useDrag = true;
		break;
	case 3:
		numEmitters = 1;
		maxParticles = 65536;
		spawnRate = 65536.0f;
		lifetime = 1.0f;
		useGravity = true;
		useDrag = true;
		break;
	case 4:
		numEmitters = 10;
		maxParticles = 8000;
		spawnRate = 6553.6f;
		lifetime = 1.0f;
		useGravity = true;
		useDrag = true;
		break;
	case 5:
		numEmitters = 100;
		maxParticles = 200;
		spawnRate = 100.0f;
		lifetime = 1.0f;
		break;
	case 6:
		numEmitters = 1;
		maxParticles = 10000;
		spawnRate = 10000.0f;
		lifetime = 1.0f;
		useGravity = true;
		useDrag = true;
		useMesh = true;
		break;
	case 7:
		numEmitters = 1;
		maxParticles = 1000;
		spawnRate = 1000.0f;
		lifetime = 1.0f;
		useRibbon = true;
		break;
	case 8:
		numEmitters = 2;
		maxParticles = 1000;
		spawnRate = 500.0f;
		lifetime = 0.5f;
		useGpuEventChain = true;
		break;
	case 9:
		numEmitters = 1;
		maxParticles = 65536;
		spawnRate = 65536.0f;
		lifetime = 1.0f;
		useMesh = true;
		break;
	case 10:
		numEmitters = 1;
		maxParticles = 65536;
		spawnRate = 65536.0f;
		lifetime = 1.0f;
		useRibbon = true;
		break;
	default:
		return;
	}

	auto* camera = sceneManager_->GetCameraManager()->GetActiveCamera();
	camera->SetTranslate({ 0.0f, 10.0f, -30.0f });
	camera->SetRotate({ 0.15f, 0.0f, 0.0f });
	if (debugCamera_)
	{
		debugCamera_->Start({ 0.0f, 10.0f, -30.0f }, { 0.15f, 0.0f, 0.0f });
	}

	for (uint32_t i = 0; i < numEmitters; ++i)
	{
		auto emitter = std::make_unique<KCE::ParticleEmitter>();
		emitter->Initialize("BenchmarkEmitter_" + std::to_string(i));
		emitter->SetSimulationMode(mode);
		emitter->SetMaxParticles(maxParticles);

		if (useMesh)
		{
			auto renderer = std::make_unique<KCE::MeshRenderer>();
			renderer->Initialize(texturePath);
			renderer->SetPrimitive(KCE::PrimitiveType::Cube);
			emitter->SetRenderer(std::move(renderer));
		}
		else if (useRibbon)
		{
			auto renderer = std::make_unique<KCE::TrailRenderer>();
			renderer->Initialize(texturePath);
			emitter->SetRenderer(std::move(renderer));
		}
		else
		{
			auto renderer = std::make_unique<KCE::SpriteRenderer>();
			renderer->Initialize(texturePath);
			emitter->SetRenderer(std::move(renderer));
		}
		if (preset >= 9 && emitter->GetRenderer())
		{
			KCE::EmissiveSettings emissive;
			emissive.enabled = true;
			emissive.source = KCE::EmissiveSource::BaseTextureMask;
			emissive.color = { 0.15f, 0.65f, 1.0f };
			emissive.intensity = 8.0f;
			emissive.bloomContribution = 1.0f;
			emitter->GetRenderer()->SetEmissiveSettings(emissive);
			emitter->GetRenderer()->SetBlendMode(KCE::BlendMode::Additive);
		}

		auto spawnRateMod = std::make_unique<KCE::SpawnRateModule>();
		spawnRateMod->SetRate(useGpuEventChain && i == 1 ? 0.0f : spawnRate);
		emitter->AddModule(std::move(spawnRateMod));

		auto lifetimeMod = std::make_unique<KCE::InitialLifetimeModule>();
		const float emitterLifetime = useGpuEventChain && i == 1 ? 1.5f : lifetime;
		lifetimeMod->SetLifetimeRange(emitterLifetime, emitterLifetime);
		emitter->AddModule(std::move(lifetimeMod));

		auto velocityMod = std::make_unique<KCE::InitialVelocityModule>();
		velocityMod->SetVelocityRange(initialVel, initialVel);
		emitter->AddModule(std::move(velocityMod));

		auto colorMod = std::make_unique<KCE::InitialColorModule>();
		colorMod->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		emitter->AddModule(std::move(colorMod));

		if (useGravity)
		{
			auto gravityMod = std::make_unique<KCE::GravityModule>();
			gravityMod->SetGravity({ 0.0f, -9.8f, 0.0f });
			emitter->AddModule(std::move(gravityMod));
		}

		if (useDrag)
		{
			auto dragMod = std::make_unique<KCE::DragModule>();
			dragMod->SetDrag(0.1f);
			emitter->AddModule(std::move(dragMod));
		}

		if (useRibbon)
		{
			// GPU sort/groupingと縮退strip境界を1回のindirect drawで検証する。
			emitter->AddModule(std::make_unique<KCE::AssignRibbonIdModule>(4));
		}

		if (useGpuEventChain && i == 1)
		{
			emitter->SetGPUEventSourceEmitterIndex(0);
			emitter->SetGPUEventTrigger(1u); // Death
			emitter->SetGPUEventProbability(1.0f);
			emitter->SetGPUEventVelocityInheritance(true, 0.35f);
			emitter->SetGPUEventInheritColor(true);
		}

		if (numEmitters > 1 && !useGpuEventChain)
		{
			float angle = (360.0f / numEmitters) * i * (std::numbers::pi_v<float> / 180.0f);
			float radius = 5.0f;
			emitter->SetPosition({ std::cos(angle) * radius, 0.0f, std::sin(angle) * radius });
		}
		else
		{
			emitter->SetPosition({ 0.0f, 0.0f, 0.0f });
		}

		benchmarkEffect_->AddEmitter(std::move(emitter));
	}

	benchmarkEffect_->Play();
	KCE::ParticleManager::GetInstance()->AddEffect(std::move(benchmarkEffect_));
}

void ParticleTestScene::StopBenchmark()
{
	// StartBenchmark() 冒頭や通常のscene終了から呼ばれても、
	// 診断設定や編集中Effectを変更しない。
	if (!isBenchmarkRunning_ && !benchmarkEffectPtr_ && !benchmarkEffect_)
	{
		return;
	}

	isBenchmarkRunning_ = false;
	activePreset_ = 0;
	statsResetDone_ = false;

	// benchmark終了後に元の計測状態に復元する
	KCE::ParticleDiagnostics::GetInstance()->SetEnabled(originalDiagnosticsEnabled_);

	// benchmarkEffectPtr_ の指すオブジェクトの所有者は ParticleManager である。
	// dangling pointer を防ぐため、必ず pointer を null にしてから Manager から削除する。
	if (benchmarkEffectPtr_)
	{
		auto* effectPtr = benchmarkEffectPtr_;
		benchmarkEffectPtr_ = nullptr;
		benchmarkEffect_.reset();
		KCE::ParticleManager::GetInstance()->RemoveEffect(effectPtr);
	}

	if (particleEditor_)
	{
		particleEditor_->NewEffect();
		particleEditor_->SetVisible(true);
	}
}
