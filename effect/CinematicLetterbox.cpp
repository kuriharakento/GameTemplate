#include "CinematicLetterbox.h"
#include "time/TimeManager.h"
#include "imgui/imgui.h"
#include "manager/editor/DebugUIManager.h"
#include <algorithm>

CinematicLetterbox::CinematicLetterbox() {}
CinematicLetterbox::~CinematicLetterbox()
{
#ifdef USE_IMGUI
    if (DebugUIManager::HasInstance())
    {
        DebugUIManager::GetInstance()->UnregisterDebugUI(this);
    }
#endif
}

void CinematicLetterbox::Initialize(SpriteCommon* spriteCommon, const std::string& texturePath, float screenWidth, float screenHeight)
{
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;

    // 上部のバー作成（オーバーシュート対応のため余白を含めて大きめに作成）
    topBar_ = std::make_unique<Sprite>();
    topBar_->Initialize(spriteCommon, texturePath);
    topBar_->SetSize({ screenWidth_, letterboxHeight_ + overshootMargin_ });
    topBar_->SetAnchorPoint({ 0.0f, 1.0f }); // 下端を基準にすることで、画面上部から降りてくる動きを実現
    topBar_->SetColor(color_);
    topBar_->SetPosition({ 0.0f, 0.0f });

    // 下部のバー作成（オーバーシュート対応のため余白を含めて大きめに作成）
    bottomBar_ = std::make_unique<Sprite>();
    bottomBar_->Initialize(spriteCommon, texturePath);
    bottomBar_->SetSize({ screenWidth_, letterboxHeight_ + overshootMargin_ });
    bottomBar_->SetAnchorPoint({ 0.0f, 0.0f }); // 上端を基準にすることで、画面下部から上がってくる動きを実現
    bottomBar_->SetColor(color_);
    bottomBar_->SetPosition({ 0.0f, screenHeight_ });

#ifdef USE_IMGUI
    DebugUIManager::GetInstance()->RegisterDebugUI(this, "Cinematic Letterbox", [this]() { this->ShowImGui(); }, DebugUIArea::Inspector);
#endif
}

void CinematicLetterbox::Show(float duration)
{
    // 既に表示中または表示済みの場合は早期リターン
    if (state_ == LetterboxState::Visible || state_ == LetterboxState::Showing)
        return;

    duration_ = duration;
    elapsed_ = 0.0f;
    state_ = LetterboxState::Showing;
}

void CinematicLetterbox::Hide(float duration)
{
    // 既に非表示中または非表示済みの場合は早期リターン
    if (state_ == LetterboxState::Hidden || state_ == LetterboxState::Hiding)
        return;

    duration_ = duration;
    elapsed_ = 0.0f;
    state_ = LetterboxState::Hiding;
}

void CinematicLetterbox::Update()
{
    float deltaTime = TimeManager::GetInstance().GetUIContext().deltaTime;

    // 表示アニメーション処理
    if (state_ == LetterboxState::Showing)
    {
        elapsed_ += deltaTime;
        float t = std::clamp(elapsed_ / duration_, 0.0f, 1.0f);
        progress_ = ApplyEasing(t);

        // アニメーション完了判定
        if (t >= 1.0f)
        {
            state_ = LetterboxState::Visible;
            progress_ = 1.0f;
        }

        UpdateBarPositions();
    }
    // 非表示アニメーション処理
    else if (state_ == LetterboxState::Hiding)
    {
        elapsed_ += deltaTime;
        float t = std::clamp(elapsed_ / duration_, 0.0f, 1.0f);
        progress_ = 1.0f - ApplyEasing(t); // 逆方向のイージング適用

        // アニメーション完了判定
        if (t >= 1.0f)
        {
            state_ = LetterboxState::Hidden;
            progress_ = 0.0f;
        }

        UpdateBarPositions();
    }
}

void CinematicLetterbox::Draw()
{
    // 完全に非表示の場合は描画コストを節約するため早期リターン
    if (state_ == LetterboxState::Hidden && progress_ <= 0.0f)
        return;

    if (topBar_) topBar_->Draw();
    if (bottomBar_) bottomBar_->Draw();
}

void CinematicLetterbox::SetColor(const Vector4& color)
{
    color_ = color;
    if (topBar_) topBar_->SetColor(color_);
    if (bottomBar_) bottomBar_->SetColor(color_);
}

void CinematicLetterbox::SetLetterboxHeight(float height)
{
    letterboxHeight_ = height;
    UpdateBarSizes();
    UpdateBarPositions();
}

void CinematicLetterbox::UpdateBarSizes()
{
    if (!topBar_ || !bottomBar_) return;

    // オーバーシュート分を含めたサイズに更新
    topBar_->SetSize({ screenWidth_, letterboxHeight_ + overshootMargin_ });
    bottomBar_->SetSize({ screenWidth_, letterboxHeight_ + overshootMargin_ });
}

void CinematicLetterbox::UpdateBarPositions()
{
    if (!topBar_ || !bottomBar_) return;

    // 上部のバー位置を計算（画面上端から進行度に応じて降りてくる）
    float topY = letterboxHeight_ * progress_;
    topBar_->SetPosition({ 0.0f, topY });

    // 下部のバー位置を計算（画面下端から進行度に応じて上がってくる）
    float bottomY = screenHeight_ - (letterboxHeight_ * progress_);
    bottomBar_->SetPosition({ 0.0f, bottomY });

    // スプライトの変換行列を更新
    topBar_->Update();
    bottomBar_->Update();
}

float CinematicLetterbox::ApplyEasing(float t) const
{
    // 設定されたイージングタイプに応じた補間関数を適用
    switch (easeType_)
    {
    case LetterboxEase::Linear:
        return t;
    case LetterboxEase::InSine:
        return EaseInSine(t);
    case LetterboxEase::OutSine:
        return EaseOutSine(t);
    case LetterboxEase::InOutSine:
        return EaseInOutSine(t);
    case LetterboxEase::InQuint:
        return EaseInQuint(t);
    case LetterboxEase::OutQuint:
        return EaseOutQuint(t);
    case LetterboxEase::InOutQuint:
        return EaseInOutQuint(t);
    case LetterboxEase::InCirc:
        return EaseInCirc(t);
    case LetterboxEase::OutCirc:
        return EaseOutCirc(t);
    case LetterboxEase::InOutCirc:
        return EaseInOutCirc(t);
    case LetterboxEase::InElastic:
        return EaseInElastic(t);
    case LetterboxEase::OutElastic:
        return EaseOutElastic(t);
    case LetterboxEase::InOutElastic:
        return EaseInOutElastic(t);
    case LetterboxEase::InExpo:
        return EaseInExpo(t);
    case LetterboxEase::OutExpo:
        return EaseOutExpo(t);
    case LetterboxEase::InOutExpo:
        return EaseInOutExpo(t);
    case LetterboxEase::OutQuad:
        return EaseOutQuad(t);
    case LetterboxEase::InOutQuart:
        return EaseInOutQuart(t);
    case LetterboxEase::InBack:
        return EaseInBack(t);
    case LetterboxEase::OutBack:
        return EaseOutBack(t);
    case LetterboxEase::InOutBack:
        return EaseInOutBack(t);
    case LetterboxEase::OutBounce:
        return EaseOutBounce(t);
    case LetterboxEase::InBounce:
        return EaseInBounce(t);
    case LetterboxEase::InOutBounce:
        return EaseInOutBounce(t);
    default:
        return t;
    }
}

void CinematicLetterbox::ShowImGui()
{
#ifdef USE_IMGUI
    // 状態表示
    const char* stateNames[] = { "Hidden", "Showing", "Visible", "Hiding" };
    ImGui::Text("State: %s", stateNames[static_cast<int>(state_)]);
    ImGui::Text("Progress: %.2f", progress_);
    ImGui::Text("Elapsed: %.2f / %.2f", elapsed_, duration_);

    // 位置デバッグ
    if (topBar_)
    {
        Vector2 topPos = topBar_->GetPosition();
        Vector2 topSize = topBar_->GetSize();
        ImGui::Text("Top Bar Y: %.2f (Size: %.2f)", topPos.y, topSize.y);
    }
    if (bottomBar_)
    {
        Vector2 bottomPos = bottomBar_->GetPosition();
        Vector2 bottomSize = bottomBar_->GetSize();
        ImGui::Text("Bottom Bar Y: %.2f (Size: %.2f)", bottomPos.y, bottomSize.y);
    }

    // パラメータ調整
    ImGui::Separator();
    if (ImGui::DragFloat("Letterbox Height", &letterboxHeight_, 1.0f, 50.0f, 300.0f))
    {
        SetLetterboxHeight(letterboxHeight_);
    }
    if (ImGui::DragFloat("Overshoot Margin", &overshootMargin_, 1.0f, 50.0f, 300.0f))
    {
        UpdateBarSizes();
    }
    ImGui::DragFloat("Duration", &duration_, 0.1f, 0.1f, 5.0f);

    // イージングタイプ選択
    const char* easeNames[] = {
        "Linear", "InSine", "OutSine", "InOutSine",
        "InQuint", "OutQuint", "InOutQuint",
        "InCirc", "OutCirc", "InOutCirc",
        "InElastic", "OutElastic", "InOutElastic",
        "InExpo", "OutExpo", "InOutExpo",
        "OutQuad", "InOutQuart",
        "InBack", "OutBack", "InOutBack",
        "OutBounce", "InBounce", "InOutBounce"
    };
    int currentEase = static_cast<int>(easeType_);
    if (ImGui::Combo("Ease Type", &currentEase, easeNames, IM_ARRAYSIZE(easeNames)))
    {
        easeType_ = static_cast<LetterboxEase>(currentEase);
    }

    // 色設定
    float color[4] = { color_.x, color_.y, color_.z, color_.w };
    if (ImGui::ColorEdit4("Color", color))
    {
        SetColor({ color[0], color[1], color[2], color[3] });
    }

    // コントロールボタン
    ImGui::Separator();
    if (ImGui::Button("Show"))
    {
        Show(duration_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Hide"))
    {
        Hide(duration_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Toggle"))
    {
        if (state_ == LetterboxState::Hidden || state_ == LetterboxState::Hiding)
        {
            Show(duration_);
        }
        else
        {
            Hide(duration_);
        }
    }
#endif
}
