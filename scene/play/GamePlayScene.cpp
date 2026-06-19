#include "GamePlayScene.h"
#include "engine/gameobject/manager/GameObjectManager.h"
#include <random>
#include <algorithm>

// audio
#include "audio/Audio.h"
// scene
#include "engine/scene/manager/SceneManager.h"
#include "application/ui/LevelUpUI.h"
// editor
#include "externals/imgui/imgui.h"
#include "manager/editor/DebugUIManager.h"
// input
#include "input/Input.h"
// math
#include "math/MatrixFunc.h"
#include "math/VectorColorCodes.h"
// graphics
#include "manager/graphics/LineManager.h"
#include "manager/effect/PostProcessManager.h"
#include "engine/graphics/3d/Object3dCommon.h"
#include "engine/manager/scene/LightManager.h"
#include "engine/manager/scene/CameraManager.h"
#include "engine/graphics/3d/InstancedModelRenderer.h"
#include "engine/manager/graphics/ShadowMapManager.h"
#include "graphics/2d/SpriteCommon.h"

// app
#include "application/game/ScoreManager.h"
#include "engine/scene/factory/SceneNames.h"
#include "engine/gameobject/component/collision/CollisionManager.h"
// components
#include "application/combo/ComboManager.h"
#include "application/GameObject/component/action/PistolComponent.h"
#include "time/TimeManager.h"
#include "application/effect/BulletTrailManager.h"
#include "effects/particle/ParticleManager.h"
#include "effects/particle/module/spawn/SpawnShapeModules.h" // SpawnShapeModule
#include "application/UI/PoseMenu.h"
#include "base/WinApp.h"

// ECS Integration
#include "engine/ecs/system/HierarchySystem.h"
#include "application/ecs/systems/EnemyBehaviorSystem.h"
#include "engine/ecs/system/InstancedRenderSystem.h"
#include "engine/ecs/components/TransformComponent.h"
#include "engine/ecs/components/HierarchyComponent.h"
#include "engine/ecs/components/TagComponent.h" // ecs::TagComponent
#include "engine/ecs/components/MovementComponent.h"
#include "engine/ecs/components/ColliderComponent.h"
#include "application/ecs/components/PlayerProgressionComponent.h"
#include "application/ecs/components/SkillComponent.h"
#include "application/ecs/components/DodgeComponent.h"
#include "application/ecs/components/ProjectileComponent.h"
#include "application/ecs/components/ImpactChargeComponent.h"
#include "application/ecs/components/EnemyStateComponent.h"
#include "application/ecs/components/StatusComponent.h"
#include "engine/ecs/components/EnemyAIComponent.h"
#include "engine/ecs/components/InstancedRenderComponent.h"
#include "engine/ecs/components/CollisionResponseComponent.h"
#include "application/ecs/components/InducedExplosionComponent.h"
#include "engine/ecs/components/LifetimeComponent.h"
#include "engine/ecs/components/Object3dComponent.h"
#include "application/ecs/components/ObstacleComponent.h"
#include "application/ecs/components/BulletComponent.h"
#include "application/ecs/components/PlayerComponent.h"
#include "application/ecs/components/EnemyTypeComponent.h"
#include "application/ecs/components/EnemyChargerComponent.h"
#include "application/ecs/CollisionConfig.h"
#include "application/ecs/components/PlayerProgressionComponent.h"
#include "application/ecs/components/SkillComponent.h"
#include "application/ecs/components/StatusComponent.h"
#include "engine/ecs/components/TagComponent.h"
#include "engine/ecs/components/TransformComponent.h"
#include "engine/ecs/components/ColliderComponent.h"
#include "engine/ecs/components/WorldBoundaryComponent.h"

// Systems
#include "engine/ecs/system/HierarchySystem.h"
#include "engine/ecs/system/MovementSystem.h"
#include "engine/ecs/system/CollisionSystem.h"
#include "engine/ecs/system/LifetimeSystem.h"
#include "engine/ecs/system/PlayerSystem.h"
#include "application/ecs/systems/PlayerActionSystem.h"
#include "application/ecs/systems/ProgressionSystem.h"
#include "application/ecs/systems/ProjectileSystem.h"
#include "application/ecs/systems/AnnihilationSystem.h"
#include "engine/ecs/system/EcsStatusSystem.h"
#include "engine/ecs/system/WorldBoundarySystem.h"
#include "engine/ecs/system/Object3dSystem.h"
#include "application/ecs/systems/TurretSystem.h"
#include "application/ecs/systems/SprinklerSystem.h"
#include "application/ecs/components/TurretComponent.h"
#include "application/ecs/components/DamageStackComponent.h"
#include "application/ecs/components/SprinklerComponent.h"
#include "application/UI/SkillSelectionUI.h"
#include "application/effect/HomingTrailManager.h"
#include "application/ecs/components/DecoyComponent.h"
#include "application/ecs/systems/DecoySystem.h"

using namespace ecs;
using namespace GameObjectComponent;

void GamePlayScene::Initialize()
{
	GameObjectManager::GetInstance()->Initialize();
	// --- エンジン・基盤の初期化 ---
	DirectionalLight mainLight;
	mainLight.direction = kLightDirection;
	mainLight.intensity = kLightIntensity;
	mainLight.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // デフォルト色
	sceneManager_->GetLightManager()->SetDirectionalLight(mainLight);
	sceneManager_->GetObject3dCommon()->SetDefaultLightManager(sceneManager_->GetLightManager());

	skydome_ = std::make_unique<Object3d>();
	skydome_->Initialize(sceneManager_->GetObject3dCommon());
	skydome_->SetModel("skydome");
	skydome_->SetScale({ kSkydomeScale, kSkydomeScale, kSkydomeScale });
	skydome_->SetLightManager(sceneManager_->GetLightManager());

	ground_ = std::make_unique<Object3d>();
	ground_->Initialize(sceneManager_->GetObject3dCommon());
	ground_->SetModel("terrain");
	ground_->SetLightManager(sceneManager_->GetLightManager());

	// --- ECS の初期化 ---
	registry_ = std::make_unique<Registry>();
	registry_->Initialize(10000);

	// コンポーネント登録
	registry_->RegisterComponent<TransformComponent>(10000);
	registry_->RegisterComponent<HierarchyComponent>(10000);
	registry_->RegisterComponent<ecs::TagComponent>(10000);
	registry_->RegisterComponent<MovementComponent>(10000);
	registry_->RegisterComponent<InstancedRenderComponent>(10000);
	registry_->RegisterComponent<ecs::ColliderComponent>(10000);
	registry_->RegisterComponent<PlayerProgressionComponent>(1);
	registry_->RegisterComponent<SkillComponent>(1);
	registry_->RegisterComponent<DodgeComponent>(1);
	registry_->RegisterComponent<ProjectileComponent>(10000);
	registry_->RegisterComponent<ImpactChargeComponent>(10000);
	registry_->RegisterComponent<ecs::StatusComponent>(10000);
	registry_->RegisterComponent<CollisionResponseComponent>(10000);
	registry_->RegisterComponent<PlayerComponent>(1);
	registry_->RegisterComponent<EnemyAIComponent>(5000);
	registry_->RegisterComponent<EnemyStateComponent>(5000);
	registry_->RegisterComponent<LifetimeComponent>(10000);
	registry_->RegisterComponent<ecs::InducedExplosionComponent>(5000);
	registry_->RegisterComponent<ObstacleComponent>(1000);
	registry_->RegisterComponent<BulletComponent>(10000);
	registry_->RegisterComponent<EnemyTypeComponent>(10000);   // 敵種別タグ
	registry_->RegisterComponent<EnemyChargerComponent>(5000); // 突進型コンポーネント
	registry_->RegisterComponent<WorldBoundaryComponent>(1);  // フィールド全体で1つ
	registry_->RegisterComponent<TurretComponent>(10);
	registry_->RegisterComponent<DamageStackComponent>(5000);
	registry_->RegisterComponent<SprinklerComponent>(10);
	registry_->RegisterComponent<DecoyComponent>(10);
	registry_->RegisterComponent<Object3dComponent>(1000);

    systemManager_ = std::make_unique<SystemManager>();

    // 1. 生成・スポーン系
    auto enemySpawnSystem = std::make_unique<EnemySpawnSystem>();
    enemySpawnSystem_ = enemySpawnSystem.get();
    enemySpawnSystem_->Initialize(
        sceneManager_->GetObject3dCommon(),
        sceneManager_->GetLightManager(),
        sceneManager_->GetCameraManager()
    );
    systemManager_->AddSystem(std::move(enemySpawnSystem));

    // 2. 移動・アクション系 (localPosition の更新)
    auto playerSystem = std::make_unique<PlayerSystem>();
    playerSystem->SetCameraManager(sceneManager_->GetCameraManager());
    systemManager_->AddSystem(std::move(playerSystem));

    auto playerActionSystem = std::make_unique<PlayerActionSystem>();
    playerActionSystem->SetCameraManager(sceneManager_->GetCameraManager());
    playerActionSystem->SetObject3dCommon(sceneManager_->GetObject3dCommon());
    playerActionSystem->SetSystemManager(systemManager_.get());
    systemManager_->AddSystem(std::move(playerActionSystem));

    // パーティクル定義のロード
    ParticleManager::GetInstance()->LoadEffectDefinition("enemy_death", "./Resources/json/particle/enemy_death.json");
    ParticleManager::GetInstance()->LoadEffectDefinition("E_skill", "./Resources/json/particle/E_skill.json");
    ParticleManager::GetInstance()->LoadEffectDefinition("E_explosion", "./Resources/json/particle/E_explosion.json");
    ParticleManager::GetInstance()->LoadEffectDefinition("hit_effect_ver2", "./Resources/json/particle/hit_effect_ver2.json");
    ParticleManager::GetInstance()->LoadEffectDefinition("move_range", "./Resources/json/particle/move_range.json");
    ParticleManager::GetInstance()->LoadEffectDefinition("R_skill", "./Resources/json/particle/R_skill.json");
    ParticleManager::GetInstance()->LoadEffectDefinition("Q_skill", "./Resources/json/particle/Q_skill.json");
    ParticleManager::GetInstance()->LoadEffectDefinition("level_up", "./Resources/json/particle/level_up.json");
    ParticleManager::GetInstance()->LoadEffectDefinition("turret_lazer", "./Resources/json/particle/turret_lazer.json");
    ParticleManager::GetInstance()->LoadEffectDefinition("Decoy_skill", "./Resources/json/particle/Decoy_skill.json");

    // 音声のロード
    Audio::GetInstance()->LoadWave("game", "bgm/game.wav", SoundGroup::BGM);
    Audio::GetInstance()->PlayWave("game", true);
    Audio::GetInstance()->LoadWave("enemy_kill", "se/enemy_kill.wav", SoundGroup::SE);
    Audio::GetInstance()->LoadWave("fire", "se/fire.wav", SoundGroup::SE);
    Audio::GetInstance()->LoadWave("R", "se/R.wav", SoundGroup::SE);
    Audio::GetInstance()->LoadWave("skill_lock", "se/skill_lock.wav", SoundGroup::SE);
    Audio::GetInstance()->LoadWave("explosion", "se/explosion.wav", SoundGroup::SE);

    // 移動制限範囲の可視化エフェクトを開始
    rangeEffect_ = ParticleManager::GetInstance()->Play("move_range", { 0.0f, 0.1f, 0.0f });
    if (rangeEffect_)
    {
        rangeEffect_->SetAutoRemove(false);
    }

    systemManager_->AddSystem(std::make_unique<EnemyBehaviorSystem>());
    systemManager_->AddSystem(std::make_unique<MovementSystem>());
    systemManager_->AddSystem(std::make_unique<ProjectileSystem>());
    auto progressionSystem = std::make_unique<ProgressionSystem>();
    auto progressionSystemPtr = progressionSystem.get();
    systemManager_->AddSystem(std::move(progressionSystem));
    systemManager_->AddSystem(std::make_unique<WorldBoundarySystem>());

    // タレット・スプリンクラーシステム
    auto turretSystem = std::make_unique<TurretSystem>();
    turretSystem->SetSystemManager(systemManager_.get());
    systemManager_->AddSystem(std::move(turretSystem));

    // 3. 行列更新・物理計算 (worldMatrix の構築)
    systemManager_->AddSystem(std::make_unique<HierarchySystem>());

    // 4. 衝突判定・解決
    systemManager_->AddSystem(std::make_unique<CollisionSystem>());

    // 5. 状態更新・ライフサイクル
    systemManager_->AddSystem(std::make_unique<AnnihilationSystem>());
    systemManager_->AddSystem(std::make_unique<DecoySystem>());
    systemManager_->AddSystem(std::make_unique<LifetimeSystem>());
    systemManager_->AddSystem(std::make_unique<EcsStatusSystem>());
    
    auto object3dSystem = std::make_unique<Object3dSystem>();
    object3dSystem_ = object3dSystem.get();
    systemManager_->AddSystem(std::move(object3dSystem));

	// Drawing system
	
	// --- トレイルマネージャー初期化 ---
	BulletTrailManager::GetInstance().Initialize();
	HomingTrailManager::GetInstance().Initialize();

	// 6. 描画準備
    systemManager_->AddSystem(std::make_unique<InstancedRenderSystem>());

	// --- モデルとインスタンスレンダラーの準備 ---
	ModelManager::GetInstance()->LoadModel("enemy");
	ModelManager::GetInstance()->LoadModel("weak_enemy", ".gltf");
	ModelManager::GetInstance()->LoadModel("tank_enemy", ".gltf");
	ModelManager::GetInstance()->LoadModel("player");
	ModelManager::GetInstance()->LoadModel("turret", ".gltf");

	Object3dCommon* obj3dCommon = sceneManager_->GetObject3dCommon();

	// 敵用レンダラー (共通・互換用)
	Model* enemyModel = ModelManager::GetInstance()->FindModel("enemy");
	if (enemyModel)
	{
		auto renderer = std::make_unique<InstancedModelRenderer>(5000); // 最大5000体
		renderer->Initialize(
			obj3dCommon->GetDXCommon(),
			obj3dCommon->GetSrvManager(),
			enemyModel
		);
		instancedRenderers_["enemy"] = std::move(renderer);
	}

	// 近接型敵用レンダラー
	Model* weakEnemyModel = ModelManager::GetInstance()->FindModel("weak_enemy");
	if (weakEnemyModel)
	{
		auto renderer = std::make_unique<InstancedModelRenderer>(5000);
		renderer->Initialize(
			obj3dCommon->GetDXCommon(),
			obj3dCommon->GetSrvManager(),
			weakEnemyModel
		);
		instancedRenderers_["weak_enemy"] = std::move(renderer);
	}

	// 突進型敵用レンダラー
	Model* tankEnemyModel = ModelManager::GetInstance()->FindModel("tank_enemy");
	if (tankEnemyModel)
	{
		auto renderer = std::make_unique<InstancedModelRenderer>(1000); // 突進型は少なめ
		renderer->Initialize(
			obj3dCommon->GetDXCommon(),
			obj3dCommon->GetSrvManager(),
			tankEnemyModel
		);
		instancedRenderers_["tank_enemy"] = std::move(renderer);
	}

	// タレット用レンダラー
	Model* turretModel = ModelManager::GetInstance()->FindModel("turret");
	if (turretModel)
	{
		auto renderer = std::make_unique<InstancedModelRenderer>(100); // 最大100体あれば十分
		renderer->Initialize(
			obj3dCommon->GetDXCommon(),
			obj3dCommon->GetSrvManager(),
			turretModel
		);
		instancedRenderers_["turret"] = std::move(renderer);
	}

	// プレイヤー用レンダラー
	Model* playerModel = ModelManager::GetInstance()->FindModel("chicken");
	if (playerModel)
	{
		auto renderer = std::make_unique<InstancedModelRenderer>(1); // 1体
		renderer->Initialize(
			obj3dCommon->GetDXCommon(),
			obj3dCommon->GetSrvManager(),
			playerModel
		);
		instancedRenderers_["chicken"] = std::move(renderer);
	}

	// --- プレイヤーEntityの生成 ---
	playerEntity_ = registry_->CreateEntity();
	if (playerEntity_ != kInvalidEntity)
	{
		ecs::TagComponent playerTag;
		playerTag.type = ecs::TagComponent::Type::Player;
		registry_->AddComponent<ecs::TagComponent>(playerEntity_, playerTag);

		registry_->AddComponent<TransformComponent>(playerEntity_, { {0.0f, 1.0f, 0.0f}, {0,0,0}, { 1.5f,1.5f,1.5f } });
		registry_->AddComponent<PlayerProgressionComponent>(playerEntity_, {});

		SkillComponent skill;
		// 初期状態ではLMBのみアンロック（デフォルト値を使用）
		registry_->AddComponent<SkillComponent>(playerEntity_, skill);

		registry_->AddComponent<DodgeComponent>(playerEntity_, {});
		registry_->AddComponent<ecs::StatusComponent>(playerEntity_, ecs::StatusComponent{});

		// コライダー設定
		ecs::ColliderComponent col;
		col.type_ = ColliderType::Sphere;
		col.sphere_.radius = 1.0f;

		// フィルタリング設定
		col.layer = CollisionLayer::kPlayer;
		col.mask = CollisionLayer::kEnemy;

		registry_->AddComponent<ecs::ColliderComponent>(playerEntity_, col);

		registry_->AddComponent<CollisionResponseComponent>(playerEntity_, {});

		// プレイヤーコンポーネント追加
		registry_->AddComponent<PlayerComponent>(playerEntity_, {});

		// 描画コンポーネント追加
		InstancedRenderComponent render;
		render.modelName_ = "chicken";
		registry_->AddComponent<InstancedRenderComponent>(playerEntity_, render);
		// 移動制限を追加 (100.0f)
		// 容量は1なので、プレイヤー以外には付与できない（メモリ最小限）
		registry_->AddComponent<WorldBoundaryComponent>(playerEntity_, { 100.0f, true });
	}

	// --- プレイヤーのステータスJSON編集の初期化 ---
	if (playerEntity_ != kInvalidEntity)
	{
		playerStatusEditor_ = std::make_unique<PlayerStatusEditor>(registry_.get(), playerEntity_);
		playerStatusEditor_->LoadJson("PlayerStatus.json"); // Resources/json/ は内部で補完される
		playerStatusEditor_->ApplyToECS();
		JsonEditor::GetInstance()->Register("PlayerStatus", playerStatusEditor_.get());
	}

	// --- 敵管理の初期化 ---
	enemyManager_ = std::make_unique<EnemyManager>();

	// 引数の型を明示的に渡す (コンパイラの推論エラー対策)
	Registry* reg = registry_.get();
	SystemManager* sys = systemManager_.get();
	Object3dCommon* objCommon = sceneManager_->GetObject3dCommon();
	SpriteCommon* sprCommon = sceneManager_->GetSpriteCommon();
	CameraManager* cam = sceneManager_->GetCameraManager();
	LightManager* light = sceneManager_->GetLightManager();
	ShadowMapManager* shadow = sceneManager_->GetShadowMapManager();

	enemyManager_->Initialize(
		reg,
		sys,
		objCommon,
		sprCommon,
		cam,
		light,
		shadow,
		nullptr
	);

	// --- カメラの初期化 ---
	Camera* activeCam = sceneManager_->GetCameraManager()->GetActiveCamera();

	topDownCamera_ = std::make_unique<TopDownCamera>();
	topDownCamera_->Initialize(sceneManager_->GetCameraManager()->GetActiveCamera());
	topDownCamera_->SetActive(true); // [BNS-Fix] カメラの更新を有効化
	constexpr float kTopDownOffsetX = -45.0f;
	constexpr float kTopDownOffsetZ = -28.0f;
	topDownCamera_->SetOffset({ kTopDownOffsetX, 0.0f, kTopDownOffsetZ });
	topDownCamera_->SetPitch(kTopDownCameraPitch);
	topDownCamera_->SetYaw(kTopDownCameraYaw);
	topDownCamera_->SetHeight(kTopDownCameraHeight);

	orbitCamera_ = std::make_unique<OrbitCameraWork>();
	orbitCamera_->Initialize(activeCam);

	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize(activeCam);

	splineCamera_ = std::make_unique<SplineCamera>();
	splineCamera_->Initialize(activeCam);

	// --- UI・演出の初期化 ---
	reticle_ = std::make_unique<Cursor>();
	reticle_->Initialize(sceneManager_->GetSpriteCommon(), "./Resources/UI/reticle.png");

	controlsGuide_ = std::make_unique<ControlsGuide>();
	controlsGuide_->Initialize(sceneManager_->GetSpriteCommon(), registry_.get(), playerEntity_);

	controlsGuide_->SetVisible(true);

	levelUpUI_ = std::make_unique<LevelUpUI>();
	levelUpUI_->Initialize(sceneManager_->GetSpriteCommon());

	// スキル選択UI
	skillSelectionUI_ = std::make_unique<SkillSelectionUI>();
	skillSelectionUI_->Initialize(sceneManager_->GetSpriteCommon());

	// 制限時間表示用 NumberSprite の初期化
	timerSprite_ = std::make_unique<NumberSprite>();
	timerSprite_->Initialize(sprCommon, "./Resources/numbers.png", { kTimerDigitWidth, kTimerDigitHeight });
	timerSprite_->SetPosition({ kTimerPosX, kTimerPosY });
	timerSprite_->SetSpacing(kTimerSpacing);
	timerSprite_->SetScale(kTimerScale);
	timerSprite_->SetAlignment(NumberAlignment::Center);
	timerSprite_->SetMinDigits(3); // 最低3桁表示（例: 060）

	// スコアラベルの初期化
	scoreLabelFontSprite_ = std::make_unique<FontSprite>();
	scoreLabelFontSprite_->Initialize(sprCommon, "nico");
	scoreLabelFontSprite_->SetText("SCORE");
	scoreLabelFontSprite_->SetAlignment(FontAlignment::Center);
	scoreLabelFontSprite_->SetSpacing(kScoreLabelSpacing);
	scoreLabelFontSprite_->SetPosition({ kScorePosX, kScoreLabelPosY });
	scoreLabelFontSprite_->SetScale(kScoreScale);

	// スコア表示用 NumberSprite の初期化
	scoreNumberSprite_ = std::make_unique<NumberSprite>();
	scoreNumberSprite_->Initialize(sprCommon, "./Resources/numbers.png", { kTimerDigitWidth, kTimerDigitHeight });
	scoreNumberSprite_->SetPosition({ kScorePosX, kScoreNumberPosY });
	scoreNumberSprite_->SetSpacing(kScoreNumberSpacing);
	scoreNumberSprite_->SetScale(kScoreNumberScale);
	scoreNumberSprite_->SetAlignment(NumberAlignment::Center);
	scoreNumberSprite_->SetMinDigits(1); 

    // ProgressionSystem に通知先を設定
    progressionSystemPtr->SetLevelUpUI(levelUpUI_.get());
    progressionSystemPtr->SetPostProcessManager(sceneManager_->GetPostProcessManager());
    progressionSystemPtr->SetSkillSelectionUI(skillSelectionUI_.get());


	// ポーズメニューの初期化
	poseMenu_ = std::make_unique<PoseMenu>();
	poseMenu_->Initialize(sceneManager_->GetSpriteCommon());
	poseMenu_->SetOnResumeCallback([this]() { poseMenu_->SetPaused(false); });
	poseMenu_->SetOnRetryCallback([this]() {
		sceneManager_->ChangeScene(SceneNames::GamePlay);
	});
	poseMenu_->SetOnTitleCallback([this]() {
		sceneManager_->ChangeScene(SceneNames::Title);
	});
	poseMenu_->SetOnExitCallback([this]() {
		PostQuitMessage(0); // ゲーム終了
	});

	// シーン遷移エフェクトの初期化
	const std::string transitionPath = "./Resources/black.png";
	const float sW = static_cast<float>(WinApp::kClientWidth);
	const float sH = static_cast<float>(WinApp::kClientHeight);

	transitionEffect_.Initialize(
		sprCommon,
		transitionPath,
		static_cast<int>(kTransitionGridX),
		static_cast<int>(kTransitionGridY),
		sW,
		sH
	);

	// シネマティックレターボックスの初期化
	cinematicLetterbox_.Initialize(
		sprCommon,
		transitionPath,
		sW,
		sH
	);

	// スコア管理のリセット
	ScoreManager::Reset();

#ifdef USE_IMGUI
	// デバッグUIの登録
	DebugUIManager::GetInstance()->RegisterDebugUI(this, "Player Stats & ECS Hub", [this]() {
		if (registry_)
		{
			ImGui::Text("Active Entities: %d", registry_->GetActiveEntityCount());
			ImGui::Separator();
		}

		// 1. プレイヤーステータス
		if (registry_ && playerEntity_ != kInvalidEntity)
		{
			if (ImGui::CollapsingHeader("Player Stats", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// HP (StatusComponent)
				if (registry_->HasComponent<ecs::StatusComponent>(playerEntity_))
				{
					auto& status = registry_->GetComponent<ecs::StatusComponent>(playerEntity_);
					float hp = status.hp_.GetValue();
					float maxHp = status.maxHp_.GetValue();
					ImGui::Text("HP: %.1f / %.1f", hp, maxHp);
					ImGui::ProgressBar(hp / maxHp, ImVec2(-1.0f, 0.0f));
				}

				// レベル・経験値 (PlayerProgressionComponent)
				if (registry_->HasComponent<PlayerProgressionComponent>(playerEntity_))
				{
					auto& prog = registry_->GetComponent<PlayerProgressionComponent>(playerEntity_);
					ImGui::Text("Level: %d", prog.level_);
					ImGui::Text("Exp: %.1f / %.1f", prog.currentExp_, prog.nextLevelExp_);
					ImGui::ProgressBar(prog.currentExp_ / prog.nextLevelExp_, ImVec2(-1.0f, 0.0f));
				}
			}

			// 2. スキル情報
			if (registry_->HasComponent<SkillComponent>(playerEntity_))
			{
				if (ImGui::CollapsingHeader("Skills", ImGuiTreeNodeFlags_DefaultOpen))
				{
					auto& skill = registry_->GetComponent<SkillComponent>(playerEntity_);

					// LMB
					ImGui::Text("LMB: %s (Timer: %.2f, DmgMul: %.2f, CDMul: %.2f)",
						skill.isLmbUnlocked_ ? "Unlocked" : "Locked",
						skill.lmbTimer_ > 0 ? skill.lmbTimer_ : 0.0f,
						skill.lmbDamageMultiplier_, skill.lmbCooldownMultiplier_);

					// Skill 1 (Q) - Base
					const char* routeName = "None";
					if (skill.route_ == SkillRoute::Bomb) routeName = "Bomb";
					else if (skill.route_ == SkillRoute::Turret) routeName = "Turret";
					ImGui::Text("Q(Base): %s (Timer: %.2f)", routeName, skill.baseSkillTimer_ > 0 ? skill.baseSkillTimer_ : 0.0f);

					// Skill 2 (E) - Special
					const char* specialName = "None";
					switch (skill.special_)
					{
					case SkillSpecialChoice::HomingMissile: specialName = "Homing Missile"; break;
					case SkillSpecialChoice::DecoyBomb: specialName = "Decoy Bomb"; break;
					case SkillSpecialChoice::MissileSalvo: specialName = "Turret: Salvo"; break;
					case SkillSpecialChoice::PlasmaLaser: specialName = "Turret: Laser"; break;
					default: break;
					}
					ImGui::Text("E(Special): %s (Timer: %.2f)", specialName, skill.specialSkillTimer_ > 0 ? skill.specialSkillTimer_ : 0.0f);

					ImGui::Separator();
					ImGui::Text("LMB Stats: Pierce: %d, Count: %d, Rate: %.2f", skill.lmbPierceCount_, skill.lmbBulletCount_, skill.lmbCooldownMultiplier_);
					ImGui::Text("Q Stats: Range: %.1f, CD: %.2f", skill.qRange_, skill.qCooldownMultiplier_);
					ImGui::Text("Turret Stats: Max: %d, Rate: %.2f", skill.qMaxTurrets_, skill.qTurretFireRateMult_);

					if (skill.isTurretBuffActive_) ImGui::TextColored({ 1,1,0,1 }, "TURRET BUFF ACTIVE (%.1fs)", skill.turretBuffTimer_);

					// Beam (R)
					ImGui::Text("R(Beam): Charge: %.1f / %.1f %s",
						skill.beamCharge_, SkillComponent::kBeamChargeMax,
						skill.isBeamReady_ ? "[READY]" : "");
					ImGui::ProgressBar(skill.beamCharge_ / SkillComponent::kBeamChargeMax, ImVec2(-1.0f, 0.0f));
				}
			}
		}

		// 3. フィールド制限設定
		if (registry_ && playerEntity_ != kInvalidEntity)
		{
			if (registry_->HasComponent<WorldBoundaryComponent>(playerEntity_))
			{
				if (ImGui::CollapsingHeader("World Boundary", ImGuiTreeNodeFlags_DefaultOpen))
				{
					auto& boundary = registry_->GetComponent<WorldBoundaryComponent>(playerEntity_);
					ImGui::SliderFloat("Boundary Radius", &boundary.radius_, 0.0f, 1000.0f);
					ImGui::Checkbox("Boundary Active", &boundary.active_);
				}
			}
		}

		// 4. スポーン設定
		if (enemySpawnSystem_)
		{
			if (ImGui::CollapsingHeader("Spawn Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				float inner = enemySpawnSystem_->GetInnerRadius();
				float outer = enemySpawnSystem_->GetOuterRadius();

				if (ImGui::SliderFloat("Inner Radius", &inner, 0.0f, 50.0f))
				{
					enemySpawnSystem_->SetInnerRadius(inner);
				}
				if (ImGui::SliderFloat("Outer Radius", &outer, inner, 100.0f))
				{
					enemySpawnSystem_->SetOuterRadius(outer);
				}
			}
		}

		// ポーズメニューのImGui
		if (poseMenu_)
		{
			poseMenu_->DrawImGui();
		}
	});
#endif

	// 初期ステートをセットして起動
	StartState(SceneState::Enter);
}

void GamePlayScene::Finalize()
{
	GameObjectManager::GetInstance()->Finalize();
	Audio::GetInstance()->StopWave("game");
	BulletTrailManager::GetInstance().Clear();
	HomingTrailManager::GetInstance().Clear();
	if (registry_) registry_.reset();
}

void GamePlayScene::Draw3D()
{
	GameObjectManager::GetInstance()->Draw3D(sceneManager_->GetCameraManager());
	skydome_->Draw();
	ground_->Draw();

	// --- インスタンス描画 (一括呼び出し) ---
	InstancedRenderSystem::DrawGrouped(
		*registry_,
		instancedRenderers_,
		sceneManager_->GetCameraManager()->GetActiveCamera(),
		sceneManager_->GetLightManager(),
		sceneManager_->GetShadowMapManager()
	);

	// --- 単体描画 (Object3dComponent) ---
	if (object3dSystem_)
	{
		object3dSystem_->Draw(*registry_, sceneManager_->GetCameraManager()->GetActiveCamera());
	}

	// --- ECS システムの描画 (衝突判定の可視化、スポーン範囲など) ---
	if (systemManager_)
	{
		systemManager_->Draw(
			*registry_,
			sceneManager_->GetCameraManager()->GetActiveCamera(),
			sceneManager_->GetLightManager(),
			sceneManager_->GetShadowMapManager()
		);
	}
}

void GamePlayScene::DrawShadow()
{
	// 1. GameObjectManager
	GameObjectManager::GetInstance()->DrawShadow(sceneManager_->GetCameraManager()->GetActiveCamera());

	// 2. ECS (Object3dSystem)
	if (object3dSystem_)
	{
		object3dSystem_->DrawShadow(*registry_, sceneManager_->GetCameraManager()->GetActiveCamera());
	}

	// 3. ECS (InstancedRenderSystem)
	InstancedRenderSystem::DrawShadowGrouped(
		*registry_,
		instancedRenderers_,
		sceneManager_->GetCameraManager()->GetActiveCamera(),
		sceneManager_->GetShadowMapManager()
	);
}

void GamePlayScene::DrawGBuffer()
{
	// 1. GameObjectManager
	GameObjectManager::GetInstance()->DrawGBuffer(sceneManager_->GetCameraManager());

	// 2. ECS (Object3dSystem)
	if (object3dSystem_)
	{
		object3dSystem_->DrawGBuffer(*registry_, sceneManager_->GetCameraManager()->GetActiveCamera());
	}

	// 3. ECS (InstancedRenderSystem)
	InstancedRenderSystem::DrawGBufferGrouped(
		*registry_,
		instancedRenderers_,
		sceneManager_->GetCameraManager()->GetActiveCamera()
	);
}

void GamePlayScene::Draw2D()
{
	GameObjectManager::GetInstance()->Draw2D();
	DrawUI();
	transitionEffect_.Draw();
	cinematicLetterbox_.Draw();
}

void GamePlayScene::UpdateUI()
{
	if (reticle_) reticle_->Update();
	if (controlsGuide_) controlsGuide_->Update();
	if (levelUpUI_) levelUpUI_->Update();
	if (skillSelectionUI_) skillSelectionUI_->Update();
	if (poseMenu_) poseMenu_->Update();

	// 制限時間の残り秒数を更新
	if (timerSprite_)
	{
		float remaining = kGameTimeLimit - gameTime_;
		if (remaining < 0.0f) remaining = 0.0f;
		int remainingSeconds = static_cast<int>(remaining);
		timerSprite_->SetNumber(remainingSeconds);

		// 残り時間が少ないときは赤く警告
		if (remaining <= kTimerWarningThreshold)
		{
			timerSprite_->SetColor(VectorColorCodes::Red);
		}
		else
		{
			timerSprite_->SetColor(VectorColorCodes::White);
		}

		timerSprite_->Update();
	}

	// スコアの表示更新
	if (scoreLabelFontSprite_)
	{
		scoreLabelFontSprite_->Update();
	}
	if (scoreNumberSprite_)
	{
		scoreNumberSprite_->SetNumber(static_cast<int>(ScoreManager::GetScore()));
		scoreNumberSprite_->Update();
	}

	// UI状態に応じてレティクルの表示/非表示を切り替え
	// (Cursorクラス内でマウスカーソルの表示状態も連動して切り替わる)
	bool isUIActive = (skillSelectionUI_ && skillSelectionUI_->IsActive()) ||
					 (levelUpUI_ && levelUpUI_->IsActive()) ||
					 (poseMenu_ && poseMenu_->IsPaused());
	if (reticle_)
	{
		reticle_->SetVisible(!isUIActive);
	}
}

void GamePlayScene::DrawUI()
{
	bool isSkillSelecting = (skillSelectionUI_ && skillSelectionUI_->IsActive());

	if (reticle_) reticle_->Draw();
	
	// スキル選択中は操作ガイドとタイマーを隠す
	if (!isSkillSelecting) {
		if (controlsGuide_) controlsGuide_->Draw();
		if (timerSprite_) timerSprite_->Draw();
		if (scoreLabelFontSprite_) scoreLabelFontSprite_->Draw();
		if (scoreNumberSprite_) scoreNumberSprite_->Draw();
	}

	if (levelUpUI_) levelUpUI_->Draw();
	if (skillSelectionUI_) skillSelectionUI_->Draw();
	
	// ポーズメニューは最前面に表示
	if (poseMenu_) poseMenu_->Draw();
}

void GamePlayScene::DrawImGui()
{
}

void GamePlayScene::OnEnterEnter()
{
	transitionEffect_.SetFadeType(FadeType::FadeOut);
	transitionEffect_.Start(kEnterTransitionDuration, VectorColorCodes::Red, VectorColorCodes::Black);
}

void GamePlayScene::OnUpdateEnter()
{
	if (transitionEffect_.GetState() == TransitionState::Done)
	{
		ChangeState(SceneState::Playing);
	}
}

void GamePlayScene::OnExitEnter()
{
}

void GamePlayScene::OnEnterIntro()
{
	introElapsed_ = 0.0f;
}

void GamePlayScene::OnUpdateIntro()
{
	if (Input::GetInstance()->TriggerKey(DIK_P))
	{
		ChangeState(SceneState::Playing);
	}
}

void GamePlayScene::OnEnterPlaying()
{
	// プレイヤーの位置をカメラのターゲットに設定
	topDownCamera_->SetTarget(&registry_->GetComponent<TransformComponent>(playerEntity_).localPosition_);
}

void GamePlayScene::OnUpdatePlaying()
{
	if (!registry_ || !systemManager_) return;

	// ポーズ中はゲームの進行処理をスキップ
	if (poseMenu_ && poseMenu_->IsPaused()) return;

	// スキル選択中はゲームの進行を停止
	if (skillSelectionUI_ && skillSelectionUI_->IsActive()) return;

	// スキル選択待ちチェック
	if (auto progressionSys = systemManager_->GetSystem<ProgressionSystem>())
	{
		if (progressionSys->IsPendingSkillSelection() && skillSelectionUI_ && !skillSelectionUI_->IsActive())
		{
			uint32_t level = progressionSys->GetPendingSelectionLevel();
			progressionSys->ClearPendingSkillSelection(); // 早期にクリアして無限ループを防止

			if (level == 2 || (level == 3 && registry_ && registry_->HasComponent<SkillComponent>(playerEntity_) && 
				registry_->GetComponent<SkillComponent>(playerEntity_).route_ == SkillRoute::None))
			{
				// ルート選択の表示（LV2、またはLV3でルート未設定の場合のフォールバック）
				skillSelectionUI_->Show(2, "Bomb Route", "Turret Route",
					[this](int choice) {
						if (registry_ && registry_->HasComponent<SkillComponent>(playerEntity_))
						{
							auto& skill = registry_->GetComponent<SkillComponent>(playerEntity_);
							skill.route_ = (choice == 0) ? SkillRoute::Bomb : SkillRoute::Turret;
						}
					},
					"./Resources/UI/text/skill_tree/skill_01_bomb.png",
					"./Resources/UI/text/skill_tree/skill_02_turret.png");
			}
			else if (level == 3)
			{
				if (registry_ && registry_->HasComponent<SkillComponent>(playerEntity_))
				{
					auto& skill = registry_->GetComponent<SkillComponent>(playerEntity_);
					if (skill.route_ == SkillRoute::Bomb)
					{
						// ボムルートの派生 (Eスキル)
						skillSelectionUI_->Show(3, "Homing Missile", "Decoy Bomb",
							[this](int choice) {
								if (registry_ && registry_->HasComponent<SkillComponent>(playerEntity_))
								{
									auto& sk = registry_->GetComponent<SkillComponent>(playerEntity_);
									sk.special_ = (choice == 0) ? SkillSpecialChoice::HomingMissile : SkillSpecialChoice::DecoyBomb;
								}
							},
							"./Resources/UI/text/skill_tree/skill_01_01_missile.png",
							"./Resources/UI/text/skill_tree/skill_01_02_decoy.png");
					}
					else if (skill.route_ == SkillRoute::Turret)
					{
						// タレットルートの派生 (Eスキル)
						skillSelectionUI_->Show(3, "Missile Salvo", "Plasma Laser",
							[this](int choice) {
								if (registry_ && registry_->HasComponent<SkillComponent>(playerEntity_))
								{
									auto& sk = registry_->GetComponent<SkillComponent>(playerEntity_);
									sk.special_ = (choice == 0) ? SkillSpecialChoice::MissileSalvo : SkillSpecialChoice::PlasmaLaser;
								}
							},
							"./Resources/UI/text/skill_tree/skill_02_01_missile.png",
							"./Resources/UI/text/skill_tree/skill_02_02_laser.png");
					}
				}
			}
			else if (level >= 4)
			{
				TriggerUpgradeSelection();
			}
		}
	}

	// スキル選択UIを表示した場合はこのフレームのシステム更新をスキップ
	// （UI表示と同一フレームでエンティティ破棄が走るとGPUリソース競合の原因になる）
	if (skillSelectionUI_ && skillSelectionUI_->IsActive()) return;

	// GameObjectManager の更新
	GameObjectManager::GetInstance()->Update();

	// すべてのECSシステムを更新
	systemManager_->Update(*registry_);

	// 従来の管理マネージャーの更新
	enemyManager_->Update();

	// 予約されたエンティティを物理削除
	registry_->FlushGarbageCollection();

	// 簡易的になゲームオーバー判定 (プレイヤーHPが0以下)
	if (playerEntity_ != kInvalidEntity && registry_->HasComponent<ecs::StatusComponent>(playerEntity_))
	{
		auto& status = registry_->GetComponent<ecs::StatusComponent>(playerEntity_);
		if (status.hp_.GetValue() <= 0.0f)
		{
			// 最終スコア取得 (PlayerProgressionComponentから同期することも可能だが、ScoreManagerが最新)
			ChangeState(SceneState::End);
		}
	}

	// ゲームクリア判定 (時間の経過)
	gameTime_ += TimeManager::GetInstance().GetGameContext().deltaTime;
	if (gameTime_ >= kGameTimeLimit)
	{
		ChangeState(SceneState::End);
	}
}

void GamePlayScene::OnExitPlaying()
{
}

void GamePlayScene::OnEnterEnd()
{
	transitionEffect_.SetFadeType(FadeType::FadeIn);
	transitionEffect_.Start(kEnterTransitionDuration, VectorColorCodes::Red, VectorColorCodes::Black);
}

void GamePlayScene::OnUpdateEnd()
{
	if (transitionEffect_.GetState() == TransitionState::Done)
	{
		// 常にリザルト画面へ
		sceneManager_->ChangeScene(SceneNames::Result);
	}
}

void GamePlayScene::OnExitEnd()
{
}

void GamePlayScene::OnEnterExit()
{
	// FadeIn（黒が現れる ＝ シーン終了）
	transitionEffect_.SetFadeType(FadeType::FadeIn);
	transitionEffect_.Start(kExitTransitionDuration, { 0,0,0,1 }, { 0,0,0,1 });
}

void GamePlayScene::OnUpdateExit()
{
	if (transitionEffect_.GetState() == TransitionState::Done)
	{
		// 次のシーンへ
	}
}

void GamePlayScene::OnExitExit()
{
}

void GamePlayScene::CommonUpdate()
{
	transitionEffect_.Update();
	cinematicLetterbox_.Update();
	UpdateUI();
	if (playerStatusEditor_)
	{
		playerStatusEditor_->Sync();
	}

	// カメラの更新
	if (isDebugCameraActive_) debugCamera_->Update();
	else topDownCamera_->Update();

	// 背景オブジェクトの更新
	skydome_->Update();
	ground_->Update();

	// 移動制限範囲エフェクトの同期
	if (rangeEffect_ && registry_ && playerEntity_ != kInvalidEntity)
	{
		if (registry_->HasComponent<WorldBoundaryComponent>(playerEntity_))
		{
			auto& boundary = registry_->GetComponent<WorldBoundaryComponent>(playerEntity_);
			
			// 全エミッターの SpawnShapeModule を更新
			for (uint32_t i = 0; i < rangeEffect_->GetEmitterCount(); ++i)
			{
				auto* emitter = rangeEffect_->GetEmitter(i);
				if (auto* shape = emitter->GetModule<SpawnShapeModule>())
				{
					// innerRadius を境界半径に、outerRadius を +0.5 に設定
					shape->SetRadius(boundary.radius_, boundary.radius_ + 0.5f);
				}
			}

			// エフェクトの有効/無効を同期
			if (boundary.active_ && !rangeEffect_->IsPlaying()) rangeEffect_->Play();
			else if (!boundary.active_ && rangeEffect_->IsPlaying()) rangeEffect_->Stop();
		}
	}
}

void GamePlayScene::TriggerUpgradeSelection()
{
	if (!registry_ || playerEntity_ == kInvalidEntity) return;
	auto& skill = registry_->GetComponent<SkillComponent>(playerEntity_);

	struct UpgradeDef {
		std::string title;
		std::string desc;
		std::function<void()> onSelect;
		std::string iconPath;
		std::string titlePath;
	};

	std::vector<UpgradeDef> pool;

	// パス定数
	const std::string kIconBase = "./Resources/UI/text/skill_tree/upgrade/icon/";
	const std::string kTextBase = "./Resources/UI/text/skill_tree/upgrade/text/";
	const std::string kTreeBase = "./Resources/UI/text/skill_tree/";

	// 通常弾強化
	pool.push_back({ "LMB: Piercing", "Bullets pierce through enemies (+1).", [&skill](){ skill.lmbPierceCount_++; }, 
		kIconBase + "default.png", kTextBase + "pierce.png" });
	pool.push_back({ "LMB: Rapid Fire", "Increase fire rate of your primary weapon.", [&skill](){ skill.lmbCooldownMultiplier_ *= 0.50f; }, 
		kIconBase + "default.png", kTextBase + "rate.png" });
	pool.push_back({ "LMB: Multi-Shot", "Fire an additional bullet per shot.", [&skill](){ skill.lmbBulletCount_++; }, 
		kIconBase + "default.png", kTextBase + "bullet_count.png" });

	// Qスキル強化
	if (skill.route_ == SkillRoute::Bomb) {
		pool.push_back({ "Shockwave: Range", "Increase the radius of your shockwaves.", [&skill](){ skill.qRange_ *= 2.2f; }, 
			kIconBase + "bomb.png", kTextBase + "range.png" });
		pool.push_back({ "Shockwave: Fast CD", "Decrease shockwave cooldown.", [&skill](){ skill.qCooldownMultiplier_ *= 0.45f; }, 
			kIconBase + "bomb.png", kTextBase + "cooltime.png" });
	}

	// Eスキル強化
	if (skill.special_ == SkillSpecialChoice::HomingMissile)
		pool.push_back({ "Missile: Capacity", "Fire more missiles at once.", [&skill](){ skill.eMissileCount_ += 2; }, 
			kTreeBase + "skill_01_01_missile.png", kTextBase + "count.png" });
	if (skill.special_ == SkillSpecialChoice::DecoyBomb)
		pool.push_back({ "Decoy: Persistence", "Increase decoy duration.", [&skill](){ skill.eDecoyDuration_ += 20.0f; }, 
			kIconBase + "decoy.png", kTextBase + "duration.png" });
	if (skill.special_ == SkillSpecialChoice::MissileSalvo)
		pool.push_back({ "Salvo: Power", "Increase turret missile damage.", [&skill](){ skill.eSalvoDamageMult_ *= 3.0f; }, 
			kIconBase + "missile.png", kTextBase + "damage.png" });
	if (skill.special_ == SkillSpecialChoice::PlasmaLaser)
		pool.push_back({ "Laser: Rapid Fire", "Increase turret laser fire rate.", [&skill](){ skill.eLaserFireRateMult_ *= 0.35f; }, 
			kIconBase + "laser.png", kTextBase + "rate.png" });

	// タレット共通強化
	if (skill.route_ == SkillRoute::Turret) {
		pool.push_back({ "Turret: Capacity", "Deploy an additional turret.", [&skill](){ skill.qMaxTurrets_++; }, 
			kIconBase + "turret.png", kTextBase + "count.png" });
		pool.push_back({ "Turret: Fire Rate", "Increase turret global fire rate.", [&skill](){ skill.qTurretFireRateMult_ *= 0.50f; }, 
			kIconBase + "turret.png", kTextBase + "rate.png" });
	}

	// シャッフルして3つ選ぶ
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(pool.begin(), pool.end(), g);

	std::vector<UpgradeOption> options;
	for (size_t i = 0; i < (std::min)(pool.size(), (size_t)3); ++i) {
		options.push_back({ pool[i].title, pool[i].desc, pool[i].onSelect, pool[i].iconPath, pool[i].titlePath });
	}

	if (skillSelectionUI_) {
		skillSelectionUI_->ShowUpgrades(options);
	}
}