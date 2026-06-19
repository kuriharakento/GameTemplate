#pragma once
#include <memory>
#include <unordered_map>
#include <string>

// scene base
#include "engine/scene/interface/BaseScene.h"

// app
#include "application/GameObject/Combatable/character/enemy/EnemyManager.h"
#include "application/GameObject/Combatable/character/player/Player.h"
#include "application/gameObject/obstacle/ObstacleManager.h"
#include "application/stage/StageManager.h"
#include "PlayerStatusEditor.h"

// camerawork
#include "camerawork/debug/DebugCamera.h"
#include "camerawork/spline/SplineCamera.h"
#include "camerawork/topDown/TopDownCamera.h"
#include "camerawork/orbit/OrbitCameraWork.h"

// ECS Integration
#include "engine/ecs/Registry.h"
#include "engine/ecs/system/SystemManager.h"
#include "application/ecs/systems/EnemySpawnSystem.h"
#include "engine/graphics/3d/InstancedModelRenderer.h"
#include "engine/ecs/system/Object3dSystem.h"

// effects
#include "application/carnage/CarnageMode.h"
#include "application/effect/CinematicLetterbox.h"
#include "application/effect/SceneTransitionEffect.h"
#include "application/minimap/Minimap.h"
#include "effects/particle/ParticleEmitter.h"
#include "effects/particle/ParticleEffect.h"
#include "graphics/2d/NumberSprite.h"

// UI
#include "application/ui/GameUI.h"
#include "application/ui/Cursor.h"
#include "application/ui/ControlsGuide.h"
#include "application/ui/PoseMenu.h"
#include "application/ui/LevelUpUI.h"
#include "application/ui/SkillSelectionUI.h"

/**
 * @brief メインゲームプレイシーン。
 */
class GamePlayScene : public BaseScene
{
public:
    void Initialize() override;
    void Finalize() override;
    void Draw3D() override;
    void DrawShadow() override;
    void DrawGBuffer() override;
    void Draw2D() override;
    void DrawImGui() override;

protected:
    void OnEnterEnter() override;
    void OnUpdateEnter() override;
    void OnExitEnter() override;
    void OnEnterIntro() override;
    void OnUpdateIntro() override;
    void OnEnterPlaying() override;
    void OnUpdatePlaying() override;
    void OnExitPlaying() override;
    void OnEnterEnd() override;
    void OnUpdateEnd() override;
    void OnExitEnd() override;
    void OnEnterExit() override;
    void OnUpdateExit() override;
    void OnExitExit() override;
    void CommonUpdate() override;

    void UpdateUI();
    void DrawUI();

    /**
     * @brief アップグレード選択を開始する。
     */
    void TriggerUpgradeSelection();

private:
    // --- 設定定数 ---
    static constexpr float kBgmVolume = 0.2f;
    static constexpr float kSeVolume = 0.3f;
    static constexpr Vector3 kLightDirection = { -0.4f, -1.0f, 1.0f };
    static constexpr float kLightIntensity = 0.8f;
    static constexpr float kSkydomeLightIntensity = 0.5f;
    static constexpr float kSkydomeScale = 0.5f;
    static constexpr float kSplineCameraSpeed = 0.001f;
    static constexpr float kTopDownCameraPitch = 0.7f;
    static constexpr float kTopDownCameraYaw = 1.0f;
    static constexpr float kTopDownCameraHeight = 43.0f;
    static constexpr float kControlsGuideScale = 0.3f;
    static constexpr int kTransitionGridX = 22;
    static constexpr int kTransitionGridY = 16;
    static constexpr float kEnterTransitionDuration = 1.5f;
    static constexpr float kExitTransitionDuration = 2.0f;

    // --- 制限時間表示 ---
    static constexpr float kGameTimeLimit = 360.0f;
    static constexpr float kTimerDigitWidth = 64.0f;
    static constexpr float kTimerDigitHeight = 64.0f;
    static constexpr float kTimerPosX = 640.0f;
    static constexpr float kTimerPosY = 50.0f;
    static constexpr float kTimerSpacing = -5.0f;
    static constexpr float kTimerScale = 0.8f;
    // 警告閾値（秒）
    static constexpr float kTimerWarningThreshold = 30.0f;
    
    // --- スコア表示 ---
    static constexpr float kScorePosX = 150.0f;
    static constexpr float kScoreLabelPosY = 300.0f;
    static constexpr float kScoreNumberPosY = 340.0f;
    static constexpr float kScoreScale = 0.5f;
    static constexpr float kScoreNumberScale = 0.4f;
    static constexpr float kScoreLabelSpacing = -20.0f;
    static constexpr float kScoreNumberSpacing = 0.0f;

    // --- ゲームプレイ ---
    std::unique_ptr<Cursor> reticle_;
    std::unique_ptr<Object3d> skydome_;
    std::unique_ptr<Object3d> ground_;
    std::unique_ptr<EnemyManager> enemyManager_;

    // --- カメラワーク ---
    std::unique_ptr<DebugCamera> debugCamera_;
    bool isDebugCameraActive_ = false;
    std::unique_ptr<SplineCamera> splineCamera_;
    std::unique_ptr<TopDownCamera> topDownCamera_;
    std::unique_ptr<OrbitCameraWork> orbitCamera_;

    // --- ECS Integration ---
    std::unique_ptr<Registry> registry_;
    std::unique_ptr<SystemManager> systemManager_;
    // システム本体は systemManager_ が所有。ここでは非所有参照を保持。
    EnemySpawnSystem* enemySpawnSystem_ = nullptr;
    class Object3dSystem* object3dSystem_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<InstancedModelRenderer>> instancedRenderers_;
    EntityID playerEntity_ = kInvalidEntity;
    std::unique_ptr<PlayerStatusEditor> playerStatusEditor_;

    // --- 演出 ---
    SceneTransitionEffect transitionEffect_;
    CinematicLetterbox cinematicLetterbox_;
    float gameTime_ = 0.0f;

    // 制限時間の数値表示
    std::unique_ptr<NumberSprite> timerSprite_;
    // スコアラベル表示
    std::unique_ptr<FontSprite> scoreLabelFontSprite_;
    // スコアの数値表示
    std::unique_ptr<NumberSprite> scoreNumberSprite_;

    // --- UI ---
    std::unique_ptr<ControlsGuide> controlsGuide_ = nullptr;
    std::unique_ptr<PoseMenu> poseMenu_ = nullptr;
    std::unique_ptr<LevelUpUI> levelUpUI_ = nullptr;
    std::unique_ptr<SkillSelectionUI> skillSelectionUI_ = nullptr;

    // --- イントロ演出 ---
    float introElapsed_ = 0.0f;
    float introDuration_ = 2.0f;
    Vector3 cameraInitialPosition_ = { -52.0f, 41.0f, 88.0f };
    Vector3 cameraInitialRotation_ = { 0.375f, 1.61f, 0.0f };

    // 制限範囲の可視化エフェクト。ParticleManager が所有
    ParticleEffect* rangeEffect_ = nullptr;
};
