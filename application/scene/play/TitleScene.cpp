#include "TitleScene.h"

// engine/ecs
#include "engine/ecs/components/TransformComponent.h"
#include "engine/ecs/components/InstancedRenderComponent.h"
#include "engine/ecs/system/InstancedRenderSystem.h"
#include "engine/ecs/system/HierarchySystem.h"

// engine/graphics
#include "engine/manager/graphics/ModelManager.h"
#include "engine/graphics/3d/InstancedModelRenderer.h"
#include "engine/graphics/3d/Object3dCommon.h"

// audio
#include "audio/Audio.h"
// scene
#include "engine/scene/manager/SceneManager.h"
// input
#include "input/Input.h"
// graphics / manager
#include "manager/effect/PostProcessManager.h"
#include "manager/graphics/LineManager.h"
#include <effects/particle/ParticleManager.h>
#include "effects/particle/ParticleEffect.h"
#ifdef USE_IMGUI
#include "manager/editor/DebugUIManager.h"
#endif

using namespace ecs;

void TitleScene::Initialize()
{
    Audio::GetInstance()->LoadWave("title", "bgm/title.wav", SoundGroup::BGM);
    Audio::GetInstance()->PlayWave("title", true);
    Audio::GetInstance()->SetVolume("title", kBgmVolume);
    Audio::GetInstance()->LoadWave("start_se", "se/tap.wav", SoundGroup::SE);
    Audio::GetInstance()->LoadWave("explosion", "se/explosion.wav", SoundGroup::SE);

    // パーティクル準備
    ParticleManager::GetInstance()->LoadEffectDefinition("title_particle", "./Resources/json/particle/title_particle.json");
    ParticleManager::GetInstance()->LoadEffectDefinition("title_direction", "./Resources/json/particle/title_direction.json");
    ParticleManager::GetInstance()->Play("title_particle", Vector3());

    // ライト調整
    DirectionalLight dirLight = sceneManager_->GetLightManager()->GetDirectionalLight();
    dirLight.direction = kLightDirection;
    dirLight.intensity = kLightIntensity;
    sceneManager_->GetLightManager()->SetDirectionalLight(dirLight);

    sceneManager_->GetCameraManager()->GetActiveCamera()->SetTranslate(Vector3(0.0f, kCameraHeight, kCameraInitialZ));
    sceneManager_->GetCameraManager()->GetActiveCamera()->SetRotate(Vector3());

    // --- ECS 初期化 ---
    registry_ = std::make_unique<Registry>();
    registry_->Initialize(1000);
    registry_->RegisterComponent<TransformComponent>(1000);
    registry_->RegisterComponent<InstancedRenderComponent>(1000);

    Object3dCommon* objCommon = sceneManager_->GetObject3dCommon();
    ModelManager* modelManager = ModelManager::GetInstance();

    // レンダラー準備
    auto setupRenderer = [&](const std::string& name, uint32_t count) {
        Model* model = modelManager->FindModel(name);
        if (model)
        {
            auto renderer = std::make_unique<InstancedModelRenderer>(count);
            renderer->Initialize(objCommon->GetDXCommon(), objCommon->GetSrvManager(), model);
            instancedRenderers_[name] = std::move(renderer);
        }
    };
    setupRenderer("weak_enemy", 500);
    setupRenderer("chicken", 1);

    // --- エンティティ生成 ---
    // プレイヤー
    playerEntity_ = registry_->CreateEntity();
    registry_->AddComponent<TransformComponent>(playerEntity_, { {0, 1.0f, 0}, {0,0,0}, {1,1,1} });
    registry_->AddComponent<InstancedRenderComponent>(playerEntity_, { "chicken" });

    // 敵 300体
    const int kEnemyCount = 300;
    srand(static_cast<unsigned int>(time(NULL)));
    for (int i = 0; i < kEnemyCount; ++i)
    {
        EntityID enemy = registry_->CreateEntity();
        
        float angle = (360.0f / kEnemyCount) * i * (3.14159f / 180.0f);
        float radius = 12.0f + static_cast<float>(rand() % 230) * 0.1f;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;

        Vector3 pos = { x, 1.0f, z };
        Vector3 toPlayer = Vector3(0, 0, 0) - pos;
        float rotY = atan2f(toPlayer.x, toPlayer.z);

        registry_->AddComponent<TransformComponent>(enemy, { pos, {0, rotY, 0}, {1,1,1} });
        registry_->AddComponent<InstancedRenderComponent>(enemy, { "weak_enemy" });
    }

    titleLogo_ = std::make_unique<Sprite>();
    titleLogo_->Initialize(sceneManager_->GetSpriteCommon(), "./Resources/title_logo.png");
    titleLogo_->SetPosition({ kLogoPositionX, kLogoPositionY });
    titleLogo_->SetAnchorPoint({ 0.5f, 0.5f });
    titleLogo_->SetSize({ kLogoWidth, kLogoHeight });

    skydome_ = std::make_unique<Object3d>();
    skydome_->Initialize(sceneManager_->GetObject3dCommon());
    skydome_->SetModel("skydome");
    skydome_->SetLightManager(sceneManager_->GetLightManager());
    skydome_->SetEnableLighting(true);
    skydome_->SetDirectionalLightIntensity(kSkydomeLightIntensity);
    skydome_->SetDirectionalLightDirection({ 0.0f, -1.0f, 0.0f });
    skydome_->SetScale({ 10.0f, 10.0f, 10.0f });
    skydome_->SetCastShadow(false);
    RegisterObject(skydome_.get());

    transitionEffect_.Initialize(
        sceneManager_->GetSpriteCommon(),
        "./Resources/black.png",
        kTransitionGridX, kTransitionGridY,
        WinApp::kClientWidth, WinApp::kClientHeight
    );

    // ポストエフェクト設定
    sceneManager_->GetPostProcessManager()->crtEffect_->SetEnabled(true);
    sceneManager_->GetPostProcessManager()->crtEffect_->SetCrtEnabled(true);
    sceneManager_->GetPostProcessManager()->crtEffect_->SetChromaticAberrationEnabled(true);
    sceneManager_->GetPostProcessManager()->crtEffect_->SetChromaticAberrationOffset(kChromaticAberrationOffset);

    fontSprite_ = std::make_unique<FontSprite>();
    fontSprite_->Initialize(sceneManager_->GetSpriteCommon(), "luna");
    fontSprite_->SetText("Press Click To Start");
    fontSprite_->SetPosition({ kFontPositionX, kFontPositionY });
    fontSprite_->SetScale(kFontScale);
    fontSprite_->SetColor(VectorColorCodes::Cyan);

    cameraDistance_ = 35.0f;
    cameraOrbitAngle_ = 0.0f;

    // 行列計算システムの初期化
    hierarchySystem_ = std::make_unique<HierarchySystem>();
    hierarchySystem_->Update(*registry_);

    StartState(SceneState::Playing);

#ifdef USE_IMGUI
    DebugUIManager::GetInstance()->RegisterDebugUI(this, "Title Scene", [this]() { this->DrawImGui(); }, DebugUIArea::Hierarchy);
#endif
}

void TitleScene::Finalize()
{
    ClearObjects();
    sceneManager_->GetPostProcessManager()->crtEffect_->SetEnabled(false);
    Audio::GetInstance()->StopWave("title");

#ifdef USE_IMGUI
    if (DebugUIManager::HasInstance()) {
        DebugUIManager::GetInstance()->UnregisterDebugUI(this);
    }
#endif
}

void TitleScene::OnEnterPlaying()
{
}

void TitleScene::OnUpdatePlaying()
{
    float dt = 0.0166f;

    fontSprite_->Update();

    if (!isTriggered_)
    {
        hierarchySystem_->Update(*registry_);

        bool isTrigger = false;
#ifdef _DEBUG
        isTrigger = Input::GetInstance()->IsMouseButtonTriggered(2) || Input::GetInstance()->TriggerKey(DIK_SPACE);
#else
        isTrigger = Input::GetInstance()->IsMouseButtonTriggered(0) || Input::GetInstance()->IsMouseButtonTriggered(2) || Input::GetInstance()->TriggerKey(DIK_SPACE);
#endif

        if (isTrigger)
        {
            isTriggered_ = true;
            if (Input::GetInstance()->TriggerKey(DIK_SPACE))
            {
                Audio::GetInstance()->PlayWave("explosion", false);
            }
            else
            {
                Audio::GetInstance()->PlayWave("start_se", false);
            }
            
            ParticleManager::GetInstance()->Play("title_direction", { 0, 1.0f, 0 });
            ParticleManager::GetInstance()->Play("explosion", { 0, 1.0f, 0 });

            transitionEffect_.SetEaseType(SceneTransitionEase::InSine);
            transitionEffect_.SetFadeType(FadeType::FadeIn);
            transitionEffect_.SetMode(TransitionMode::LeftTopToRightBottom);
            transitionEffect_.Start(kTransitionDuration, VectorColorCodes::Red, VectorColorCodes::Black);

            // 敵を一斉に非表示 (ECS)
            auto array = registry_->View<InstancedRenderComponent>();
            int size = static_cast<int>(array->GetSize());
            for (int i = size - 1; i >= 0; --i)
            {
                EntityID entity = array->GetEntityFromDenseIndex(static_cast<uint32_t>(i));
                if (entity != playerEntity_)
                {
                    registry_->RemoveComponent<InstancedRenderComponent>(entity);
                }
            }
        }

        // カメラオービット
        cameraOrbitAngle_ += 0.005f;
        Vector3 cameraPos = {
            cosf(cameraOrbitAngle_) * cameraDistance_,
            3.0f,
            sinf(cameraOrbitAngle_) * cameraDistance_
        };
        auto camera = sceneManager_->GetCameraManager()->GetActiveCamera();
        camera->SetTranslate(cameraPos);
        
        Vector3 toTarget = cameraTarget_ - cameraPos;
        camera->SetRotate({ atan2f(-toTarget.y, sqrtf(toTarget.x * toTarget.x + toTarget.z * toTarget.z)), atan2f(toTarget.x, toTarget.z), 0.0f });
    }
    else
    {
        // ズームアウト演出
        explosionTimer_ += dt;
        auto camera = sceneManager_->GetCameraManager()->GetActiveCamera();

        const float kZoomOutDuration = 1.2f;
        const float kInitialDist = 35.0f;
        const float kMaxDist = 65.0f;
        const float kInitialFov = 0.45f;
        const float kMaxFov = 0.55f;
        
        float t = explosionTimer_ / kZoomOutDuration;
        t = (std::min)(1.0f, t);
        
        float easeT = 1.0f - powf(1.0f - t, 3.0f);

        float currentDist = kInitialDist + (kMaxDist - kInitialDist) * easeT;
        float currentFov = kInitialFov + (kMaxFov - kInitialFov) * easeT;
        camera->SetFovY(currentFov);

        cameraOrbitAngle_ += 0.02f * (1.0f + easeT * 1.5f);

        Vector3 cameraPos = {
            cosf(cameraOrbitAngle_) * currentDist,
            3.0f + 7.0f * easeT,
            sinf(cameraOrbitAngle_) * currentDist
        };
        camera->SetTranslate(cameraPos);

        Vector3 toTarget = cameraTarget_ - cameraPos;
        camera->SetRotate({ atan2f(-toTarget.y, sqrtf(toTarget.x * toTarget.x + toTarget.z * toTarget.z)), atan2f(toTarget.x, toTarget.z), 0.0f });

        // カメラシェイク
        if (explosionTimer_ < 0.3f)
        {
            Vector3 offset = { (rand() % 100 - 50) * 0.03f, (rand() % 100 - 50) * 0.03f, (rand() % 100 - 50) * 0.03f };
            camera->SetTranslate(camera->GetTranslate() + offset);
        }

        if (transitionEffect_.GetState() == TransitionState::Done)
        {
            ChangeState(SceneState::Exit);
        }
    }

    transitionEffect_.Update();
    titleLogo_->Update();
    skydome_->Update(sceneManager_->GetCameraManager());
}

void TitleScene::OnExitPlaying()
{
}

void TitleScene::OnEnterExit()
{
}

void TitleScene::OnUpdateExit()
{
    transitionEffect_.Update();

    if (transitionEffect_.GetState() == TransitionState::Done)
    {
        if (sceneManager_)
        {
            sceneManager_->ChangeScene(SceneNames::GamePlay);
        }
    }
}

void TitleScene::OnExitExit()
{
}

void TitleScene::Draw3D()
{
    LineManager::GetInstance()->DrawGrid(
        kGridSize,
        kGridSpacing,
        VectorColorCodes::DarkGray
    );

    InstancedRenderSystem::DrawGrouped(
        *registry_,
        instancedRenderers_,
        sceneManager_->GetCameraManager()->GetActiveCamera(),
        sceneManager_->GetLightManager(),
        sceneManager_->GetShadowMapManager()
    );

    BaseScene::Draw3D();
}

void TitleScene::Draw2D()
{
    titleLogo_->Draw();
    fontSprite_->Draw();
    transitionEffect_.Draw();
}

void TitleScene::DrawImGui()
{
#ifdef USE_IMGUI
    // タイトルシーンに関する追加デバッグ情報があればここに記述
    ImGui::Text("Title Scene Active");
#endif
}
