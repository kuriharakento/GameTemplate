#include "CarnageMode.h"
#include "application/combo/ComboManager.h"
#include "application/ecs/components/PlayerComponent.h"
#include "imgui/imgui.h"
#include "time/TimeManager.h"
#include "manager/editor/DebugUIManager.h"

CarnageMode::CarnageMode(Registry* registry, EntityID playerEntity)
    : registry_(registry), playerEntity_(playerEntity)
{
    timer_ = std::make_unique<Timer>("CarnageTimer", initialTime_);
    timer_->SetOnFinish([this]() {
        RemoveBuffs();
        HideUI();
    });

#ifdef USE_IMGUI
    DebugUIManager::GetInstance()->RegisterDebugUI(this, "CarnageMode", [this]() { this->DrawImGui(); }, DebugUIArea::Inspector);
#endif
}

CarnageMode::~CarnageMode()
{
#ifdef USE_IMGUI
    if (DebugUIManager::HasInstance())
    {
        DebugUIManager::GetInstance()->UnregisterDebugUI(this);
    }
#endif
}

void CarnageMode::TryStart()
{
    int comboCount = ComboManager::GetInstance().GetComboCount();
    if (!IsActive() && comboCount >= comboThreshold_)
    {
        timer_->Reset();
        timer_->Start();
        ApplyBuffs();
        ShowUI();
    }
}

void CarnageMode::Update()
{
    timer_->Update(TimeManager::GetInstance().GetGameContext().realDeltaTime);

    TryStart();

    if (IsActive() && ComboManager::GetInstance().GetComboCount() <= 0)
    {
        timer_->Stop();
        RemoveBuffs();
        HideUI();
    }
}

void CarnageMode::ExtendTimer()
{
    if (IsActive())
    {
        float remain = timer_->GetRemainingTime();
        float duration = remain + extensionTime_;
        timer_->Reset();
        timer_->SetDuration(duration);
        timer_->Start();
    }
}

bool CarnageMode::IsActive() const
{
    return timer_ ? timer_->IsRunning() : false;
}

float CarnageMode::GetTimeLeft() const
{
    return timer_ ? timer_->GetRemainingTime() : 0.0f;
}

void CarnageMode::ApplyBuffs()
{
    if (!registry_ || playerEntity_ == kInvalidEntity) return;
    if (!registry_->HasComponent<ecs::PlayerComponent>(playerEntity_)) return;

    auto& p = registry_->GetComponent<ecs::PlayerComponent>(playerEntity_);
    p.attackMultiplier_ = 1.0f + attackUpRate_;
    p.moveSpeedMultiplier_ = 1.0f + speedUpRate_;
}

void CarnageMode::RemoveBuffs()
{
    if (!registry_ || playerEntity_ == kInvalidEntity) return;
    if (!registry_->HasComponent<ecs::PlayerComponent>(playerEntity_)) return;

    auto& p = registry_->GetComponent<ecs::PlayerComponent>(playerEntity_);
    p.attackMultiplier_ = 1.0f;
    p.moveSpeedMultiplier_ = 1.0f;
}

void CarnageMode::ShowUI() {}
void CarnageMode::HideUI() {}

#ifdef USE_IMGUI
void CarnageMode::DrawImGui()
{
    ImGui::Text("Is Active: %s", IsActive() ? "Active" : "No Active");
    ImGui::Text("Time Left: %.2f", GetTimeLeft());
    ImGui::SliderFloat("Attack Up Rate", &attackUpRate_, 0.0f, 2.0f, "%.2f");
    if (ImGui::Button("Extend Timer"))
    {
        ExtendTimer();
    }
}
#endif
