#include "Minimap.h"
#include "engine/ecs/components/TransformComponent.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "manager/editor/DebugUIManager.h"
#endif

using namespace ecs;

void Minimap::Initialize(SpriteCommon* spriteCommon, StageManager* stageManager)
{
    spriteCommon_ = spriteCommon;
    stageManager_ = stageManager;
    
    frame_ = std::make_unique<Sprite>();
    frame_->Initialize(spriteCommon_, "./Resources/minimap_frame.png");
    frame_->SetPosition(Vector2(1080.0f, 90.0f));
    frame_->SetSize({ 300.0f, 300.0f });
    frame_->SetAnchorPoint({ 0.5f, 0.5f });

    enemyIcons_.clear();

    areaIcon_.clear();

    playerIcon_ = std::make_unique<Sprite>();
    playerIcon_->Initialize(spriteCommon_, "./Resources/red.png");
    playerIcon_->SetSize({ 18.0f, 18.0f });
    playerIcon_->SetAnchorPoint({ 0.5f, 0.5f });
    playerIcon_->SetColor(VectorColorCodes::Cyan);

#ifdef USE_IMGUI
    DebugUIManager::GetInstance()->RegisterDebugUI(this, "Minimap", [this]() { this->DrawImGui(); }, DebugUIArea::Scene);
#endif
}

Minimap::~Minimap()
{
#ifdef USE_IMGUI
    if (DebugUIManager::HasInstance()) {
        DebugUIManager::GetInstance()->UnregisterDebugUI(this);
    }
#endif
}

void Minimap::DrawImGui()
{
#ifdef USE_IMGUI
    static Vector2 framePos = frame_->GetPosition();
    if (ImGui::DragFloat2("Frame Position", &framePos.x, 1.0f, 0.0f, 1920.0f))
    {
        frame_->SetPosition(framePos);
    }
    static Vector2 frameSize = frame_->GetSize();
    if (ImGui::DragFloat2("Frame Size", &frameSize.x, 1.0f, 50.0f, 400.0f))
    {
        frame_->SetSize(frameSize);
    }
#endif
}

void Minimap::Update()
{
    frame_->Update();

    float playerYaw = stageManager_->GetPlayerRotation().y;
    playerIcon_->SetPosition(frame_->GetPosition());
    playerIcon_->SetRotation(playerYaw);
    playerIcon_->Update();

    // ECSからTransformComponentを取得して位置を同期
    auto enemyManager = stageManager_->GetEnemyManager();
    auto registry = enemyManager ? enemyManager->GetRegistry() : nullptr;

    uint32_t activeEnemyCount = registry ? registry->GetActiveEntityCount() : 0;

    // 敵の数に合わせてアイコンを同動的に追加/削除
    if (enemyIcons_.size() < activeEnemyCount)
    {
        for (size_t i = enemyIcons_.size(); i < activeEnemyCount; ++i)
        {
            auto icon = std::make_unique<Sprite>();
            icon->Initialize(spriteCommon_, "./Resources/red.png");
            icon->SetSize({ 10.0f, 10.0f });
            icon->SetAnchorPoint({ 0.5f, 0.5f });
            enemyIcons_.push_back(std::move(icon));
        }
    }
    else if (enemyIcons_.size() > activeEnemyCount)
    {
        enemyIcons_.resize(activeEnemyCount);
    }

    if (registry && activeEnemyCount > 0)
    {
        size_t i = 0;
        auto view = registry->View<TransformComponent>();
        for (const auto& transform : *view)
        {
            // UIの上限または登録されているアイコン数まで描画
            if (i >= enemyIcons_.size()) break;

            // ECSのTransformから位置情報を取得
            Vector3 enemyPos = { transform.localPosition_.x, transform.localPosition_.y, transform.localPosition_.z };
            float enemyYaw = 0.0f; 

            Vector2 miniMapPos = WorldToMinimap(enemyPos);
            enemyIcons_[i]->SetPosition(miniMapPos);
            enemyIcons_[i]->SetRotation(enemyYaw);
            enemyIcons_[i]->Update();

            ++i;
        }
    }

    // エリアアイコンの動的管?E
    auto areaManager = stageManager_->GetStage()->GetAreaManager();
    const auto& areas = areaManager->GetAreas();
    
    if (areaIcon_.size() < areas.size())
    {
        for (size_t i = areaIcon_.size(); i < areas.size(); ++i)
        {
            auto icon = std::make_unique<Sprite>();
            icon->Initialize(spriteCommon_, "./Resources/black.png");
            icon->SetSize({ 20.0f, 20.0f });
            icon->SetAnchorPoint({ 0.5f, 0.5f });
            areaIcon_.push_back(std::move(icon));
            areaActiveFlags_.push_back(false);
        }
    }
    else if (areaIcon_.size() > areas.size())
    {
        areaIcon_.resize(areas.size());
    }

    for (size_t i = 0; i < areas.size(); ++i)
    {
        Vector3 areaPos = areas[i]->GetAreaObject()->GetPosition();
        float areaYaw = areas[i]->GetAreaObject()->GetRotation().y;
        Vector2 miniMapPos = WorldToMinimap(areaPos);
        areaIcon_[i]->SetPosition(miniMapPos);
        areaIcon_[i]->SetRotation(areaYaw);
        areaIcon_[i]->Update();
        areaActiveFlags_[i] = areas[i]->IsActive();
    }
}

void Minimap::Draw()
{
    frame_->Draw();

    playerIcon_->Draw();

    for (auto& icon : enemyIcons_)
    {
        icon->Draw();
    }

    for (int i = 0; i < areaIcon_.size(); ++i)
    {
        if (!areaActiveFlags_[i]) { continue; }
        areaIcon_[i]->Draw();
    }
}

Vector2 Minimap::WorldToMinimap(const Vector3& worldPos) const
{
    Vector3 playerPos = stageManager_->GetPlayerPosition();

    Vector2 frameCenter = frame_->GetPosition();
    Vector2 frameSize = frame_->GetSize();
    float halfWidth = mapWidth_ * 0.5f;
    float halfHeight = mapHeight_ * 0.5f;

    // プレイヤーからの相対座標を計算し、ミニ?EチE?Eスケールに変換
    // X軸はそ?Eまま、Z軸は反転?E?上が北になるよぁE???E?E
    float nx = ((worldPos.x - playerPos.x) / halfWidth) * (frameSize.x * 0.5f);
    float ny = -((worldPos.z - playerPos.z) / halfHeight) * (frameSize.y * 0.5f);

    float x = frameCenter.x + nx;
    float y = frameCenter.y + ny;

    frameSize.x /= 2.0f;
    frameSize.y /= 2.0f;

    // ミニマップ?E?E??篁E??外?E場合?E?E??上にクランチE
    float iconHalfSize = 5.0f;
    float radius = (frameSize.x < frameSize.y ? frameSize.x : frameSize.y) * 0.5f - iconHalfSize;

    float dx = x - frameCenter.x;
    float dy = y - frameCenter.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > radius)
    {
        float angle = std::atan2(dy, dx);
        x = frameCenter.x + radius * std::cos(angle);
        y = frameCenter.y + radius * std::sin(angle);
    }

    return Vector2(x, y);
}
