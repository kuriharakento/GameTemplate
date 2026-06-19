#pragma once
#include <memory>

// app
#include "application/gameObject/combatable/character/enemy/EnemyManager.h"
#include "application/gameObject/combatable/character/player/Player.h"
#include "application/gameObject/obstacle/ObstacleManager.h"
#include "application/stage/StageManager.h"
// camerawork
#include "camerawork/debug/DebugCamera.h"
#include "camerawork/spline/SplineCamera.h"
#include "camerawork/topDown/TopDownCamera.h"

// scene
#include "engine/scene/interface/BaseScene.h"

// graphics
#include "graphics/3d/Object3d.h"

// effects
#include "application/carnage/CarnageMode.h"
#include "application/effect/SceneTransitionEffect.h"
#include "application/minimap/Minimap.h"
#include "effects/particle/ParticleEmitter.h"
#include "graphics/2d/NumberSprite.h"
#include <graphics/2d/FontSprite.h>

#include "engine/ecs/Registry.h"
#include "engine/graphics/3d/InstancedModelRenderer.h"
#include "engine/ecs/system/HierarchySystem.h"
#include <unordered_map>

/**
 * @brief タイトルシーン。
 * 
 * プレイヤーの入力待ちとタイトル演出を行う。
 */
class TitleScene : public BaseScene
{
public:
    /**
     * @brief 初期化。
     */
    void Initialize() override;
    
    /**
     * @brief 終了処理。
     */
    void Finalize() override;
    
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
    void OnEnterPlaying() override;
    void OnUpdatePlaying() override;
    void OnExitPlaying() override;
    void OnEnterExit() override;
    void OnUpdateExit() override;
    void OnExitExit() override;

private:
    // --- 設定定数 ---
    static constexpr float kBgmVolume = 0.2f;
    static constexpr Vector3 kLightDirection = { 0.0f, -1.0f, 0.0f };
    static constexpr float kLightIntensity = 0.8f;
    static constexpr float kSkydomeLightIntensity = 0.5f;
    static constexpr float kSkydomeScale = 0.8f;
    static constexpr float kCameraMoveSpeed = 0.1f;
    static constexpr float kCameraResetZ = 100.0f;
    static constexpr float kCameraInitialZ = -15.0f;
    static constexpr float kCameraHeight = 1.5f;
    static constexpr float kLogoPositionX = 640.0f;
    static constexpr float kLogoPositionY = 100.0f;
    static constexpr float kLogoWidth = 300.0f;
    static constexpr float kLogoHeight = 200.0f;
    static constexpr float kFontPositionX = 200.0f;
    static constexpr float kFontPositionY = 600.0f;
    static constexpr float kFontScale = 0.5f;
    static constexpr int kTransitionGridX = 32;
    static constexpr int kTransitionGridY = 26;
    static constexpr float kTransitionDuration = 1.0f;
    static constexpr float kChromaticAberrationOffset = 10.0f;
    static constexpr float kCubeWaveSpeed = 0.05f;
    static constexpr float kCubeBaseY = 1.0f;
    static constexpr float kCubeAmplitude = 0.5f;
    static constexpr float kCubeRotateSpeed = 0.07f;
    static constexpr float kCubeMaxRotateY = 3.14f;
    static constexpr float kCubeDistanceFromCamera = 10.0f;
    static constexpr float kCubeOffsetY = -1.0f;
    static constexpr float kGridSize = 600.0f;
    static constexpr float kGridSpacing = 5.0f;

    // --- メンバ変数 ---
    std::unique_ptr<Sprite> titleLogo_;
    std::unique_ptr<Object3d> skydome_;

    // --- ECS 管理 ---
    std::unique_ptr<Registry> registry_;
    std::unordered_map<std::string, std::unique_ptr<InstancedModelRenderer>> instancedRenderers_;
    std::unique_ptr<HierarchySystem> hierarchySystem_; // 所有権あり
    EntityID playerEntity_ = kInvalidEntity;

    // 演出フラグとタイマー
    bool isTriggered_ = false;
    float explosionTimer_ = 0.0f;
    float cameraOrbitAngle_ = 0.0f;
    float cameraDistance_ = 12.0f;
    Vector3 cameraTarget_ = { 0, 0, 0 };
    Vector3 cameraShakeOffset_ = { 0, 0, 0 };

    // 装飾用キューブ
    OBB cube_{};
    float cubeRotateY = 0.0f;
    float cubeWaveTime = 0.0f;

    // シーン遷移エフェクト
    SceneTransitionEffect transitionEffect_;
    // フォントスプライト
    std::unique_ptr<FontSprite> fontSprite_;
};

