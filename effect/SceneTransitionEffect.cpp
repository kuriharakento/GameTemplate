#include "SceneTransitionEffect.h"
#include <algorithm>
#include <cmath>
#include "imgui/imgui.h"
#include "time/TimeManager.h"
#include "manager/editor/DebugUIManager.h"

SceneTransitionEffect::SceneTransitionEffect() {}
SceneTransitionEffect::~SceneTransitionEffect()
{
#ifdef USE_IMGUI
    if (DebugUIManager::HasInstance())
    {
        DebugUIManager::GetInstance()->UnregisterDebugUI(this);
    }
#endif
}

void SceneTransitionEffect::Initialize(SpriteCommon* spriteCommon, const std::string& texturePath, int gridX, int gridY, float screenWidth, float screenHeight)
{
    gridX_ = gridX;
    gridY_ = gridY;
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    gridSprites_.resize(gridY_);

    float cellWidth = screenWidth_ / gridX_;
    float cellHeight = screenHeight_ / gridY_;

    // グリッド状にスプライトを配置
    for (int y = 0; y < gridY_; ++y)
    {
        gridSprites_[y].resize(gridX_);
        for (int x = 0; x < gridX_; ++x)
        {
            gridSprites_[y][x] = std::make_unique<Sprite>();
            gridSprites_[y][x]->Initialize(spriteCommon, texturePath);
            gridSprites_[y][x]->SetSize({ cellWidth, cellHeight });
            gridSprites_[y][x]->SetPosition({ x * cellWidth, y * cellHeight });
            gridSprites_[y][x]->SetAnchorPoint({ 0.0f, 0.0f });
            
            // 各グリッドにグラデーションカラーを適用
            float gridProgress = CalcGridProgress(x, y);
            Vector4 color = LerpColor(startColor_, endColor_, gridProgress);
            color.w = 1.0f;
            gridSprites_[y][x]->SetColor(color);
        }
    }

#ifdef USE_IMGUI
    DebugUIManager::GetInstance()->RegisterDebugUI(this, "Scene Transition", [this]() { this->ShowImGui(); }, DebugUIArea::Inspector);
#endif
}

void SceneTransitionEffect::Start(float duration, const Vector4& startColor, const Vector4& endColor)
{
    duration_ = duration;
    elapsed_ = 0.0f;
    transitionRate_ = 0.0f;
    state_ = TransitionState::Playing;
    startColor_ = startColor;
    endColor_ = endColor;
    
    // 新しい色設定でグラデーションを再適用
    for (int y = 0; y < gridY_; ++y)
    {
        for (int x = 0; x < gridX_; ++x)
        {
            float gridProgress = CalcGridProgress(x, y);
            Vector4 color = LerpColor(startColor_, endColor_, gridProgress);
            color.w = (fadeType_ == FadeType::FadeOut) ? 1.0f : 0.0f;  // フェードタイプに応じた初期透過度
            gridSprites_[y][x]->SetColor(color);
        }
    }
}

void SceneTransitionEffect::Update()
{
    float deltaTime = TimeManager::GetInstance().GetUIContext().deltaTime;

    // シーンロード等でフレームレートが極端に落ちた時に、一気に遷移が完了してしまうのを防止するため、最大値を設定
    deltaTime = (std::min)(deltaTime, 0.1f);

    // 再生中の場合は進行度を更新
    if (state_ == TransitionState::Playing)
    {
        elapsed_ += deltaTime;
        transitionRate_ = std::clamp(elapsed_ / duration_, 0.0f, 1.0f);
        if (transitionRate_ >= 1.0f)
        {
            state_ = TransitionState::Done;
        }
    }

    // 各グリッドセルの透過度を計算して更新
    for (int y = 0; y < gridY_; ++y)
    {
        for (int x = 0; x < gridX_; ++x)
        {
            // このグリッドの開始タイミングを取得
            float gridProgress = CalcGridProgress(x, y);
            // グリッド固有の進行度を計算（全体進行度からグリッド開始点を引く）
            float fadeProgress = (transitionRate_ - gridProgress) / (1.0f - gridProgress);
            fadeProgress = std::clamp(fadeProgress, 0.0f, 1.0f);

            // イージングを適用
            float baseAlpha = ApplyEasing(fadeProgress);
            // フェードタイプに応じて透過度を反転
            float alpha = (fadeType_ == FadeType::FadeOut) ? 1.0f - baseAlpha : baseAlpha;

            Vector4 color = gridSprites_[y][x]->GetColor();
            color.w = alpha;
            gridSprites_[y][x]->SetColor(color);
            gridSprites_[y][x]->Update();
        }
    }
}

void SceneTransitionEffect::Draw()
{
    // 待機中は描画しない
    if (state_ == TransitionState::Idle)
    {
        return;
    }
    // 描画
    for (int y = 0; y < gridY_; ++y)
    {
        for (int x = 0; x < gridX_; ++x)
        {
            float alpha = gridSprites_[y][x]->GetColor().w;
            if (alpha <= 0.0f)
            {
                continue; // 透明なら描画しない
            }
            gridSprites_[y][x]->Draw();
        }
    }
}

TransitionState SceneTransitionEffect::GetState() const { return state_; }
void SceneTransitionEffect::SetState(TransitionState state) { state_ = state; }
void SceneTransitionEffect::SetEaseType(SceneTransitionEase type) { easeType_ = type; }
void SceneTransitionEffect::SetMode(TransitionMode mode) { mode_ = mode; }
void SceneTransitionEffect::SetFadeType(FadeType type) { fadeType_ = type; }

float SceneTransitionEffect::ApplyEasing(float t) const
{
    switch (easeType_)
    {
    case SceneTransitionEase::Linear: return t;
    case SceneTransitionEase::InSine: return EaseInSine<float>(t);
    case SceneTransitionEase::OutSine: return EaseOutSine<float>(t);
    case SceneTransitionEase::InOutSine: return EaseInOutSine<float>(t);
    case SceneTransitionEase::InQuint: return EaseInQuint<float>(t);
    case SceneTransitionEase::OutQuint: return EaseOutQuint<float>(t);
    case SceneTransitionEase::InOutQuint: return EaseInOutQuint<float>(t);
    case SceneTransitionEase::InCirc: return EaseInCirc<float>(t);
    case SceneTransitionEase::OutCirc: return EaseOutCirc<float>(t);
    case SceneTransitionEase::InOutCirc: return EaseInOutCirc<float>(t);
    case SceneTransitionEase::InElastic: return EaseInElastic<float>(t);
    case SceneTransitionEase::OutElastic: return EaseOutElastic<float>(t);
    case SceneTransitionEase::InOutElastic: return EaseInOutElastic<float>(t);
    case SceneTransitionEase::InExpo: return EaseInExpo<float>(t);
    case SceneTransitionEase::OutExpo: return EaseOutExpo<float>(t);
    case SceneTransitionEase::InOutExpo: return EaseInOutExpo<float>(t);
    case SceneTransitionEase::OutQuad: return EaseOutQuad<float>(t);
    case SceneTransitionEase::InOutQuart: return EaseInOutQuart<float>(t);
    case SceneTransitionEase::InBack: return EaseInBack<float>(t);
    case SceneTransitionEase::OutBack: return EaseOutBack<float>(t);
    case SceneTransitionEase::InOutBack: return EaseInOutBack<float>(t);
    case SceneTransitionEase::OutBounce: return EaseOutBounce<float>(t);
    case SceneTransitionEase::InBounce: return EaseInBounce<float>(t);
    case SceneTransitionEase::InOutBounce: return EaseInOutBounce<float>(t);
    default: return t;
    }
}

Vector4 SceneTransitionEffect::LerpColor(const Vector4& c0, const Vector4& c1, float t) const
{
    return Vector4(
        c0.x + (c1.x - c0.x) * t,
        c0.y + (c1.y - c0.y) * t,
        c0.z + (c1.z - c0.z) * t,
        1.0f // αは下で合成する
    );
}

float SceneTransitionEffect::CalcGridProgress(int x, int y) const
{
    switch (mode_)
    {
    case TransitionMode::LeftTopToRightBottom:
        return float(x + y) / float(gridX_ + gridY_ - 2);
    case TransitionMode::RightBottomToLeftTop:
        return float((gridX_ - 1 - x) + (gridY_ - 1 - y)) / float(gridX_ + gridY_ - 2);
    case TransitionMode::RightTopToLeftBottom:
        return float((gridX_ - 1 - x) + y) / float(gridX_ + gridY_ - 2);
    case TransitionMode::LeftBottomToRightTop:
        return float(x + (gridY_ - 1 - y)) / float(gridX_ + gridY_ - 2);
    case TransitionMode::TopToBottom:
        return float(y) / float(gridY_ - 1);
    case TransitionMode::BottomToTop:
        return float(gridY_ - 1 - y) / float(gridY_ - 1);
    case TransitionMode::CenterToEdges:
    {
        float cx = (gridX_ - 1) / 2.0f;
        float cy = (gridY_ - 1) / 2.0f;
        float dist = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
        float max_dist = sqrtf(cx * cx + cy * cy);
        return dist / max_dist;
    }
    case TransitionMode::EdgesToCenter:
    {
        float cx = (gridX_ - 1) / 2.0f;
        float cy = (gridY_ - 1) / 2.0f;
        float dist = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
        float max_dist = sqrtf(cx * cx + cy * cy);
        return 1.0f - dist / max_dist;
    }
    default:
        return float(x + y) / float(gridX_ + gridY_ - 2);
    }
}

void SceneTransitionEffect::ShowImGui()
{
#ifdef USE_IMGUI
    ImGui::Text("State: %s", state_ == TransitionState::Idle ? "Idle" : state_ == TransitionState::Playing ? "Playing" : "Done");
    ImGui::SliderFloat("Duration(s)", &duration_, 0.1f, 5.0f);
    static int easeIdx = static_cast<int>(easeType_);
    const char* easeNames[] = {
       "Linear", "InSine", "OutSine", "InOutSine", "InQuint", "OutQuint", "InOutQuint", "InCirc", "OutCirc",
       "InOutCirc", "InElastic", "OutElastic", "InOutElastic", "InExpo", "OutExpo", "InOutExpo", "OutQuad",
       "InOutQuart", "InBack", "OutBack", "InOutBack", "OutBounce", "InBounce", "InOutBounce"
    };
    if (ImGui::Combo("Ease Function", &easeIdx, easeNames, IM_ARRAYSIZE(easeNames)))
    {
        easeType_ = static_cast<SceneTransitionEase>(easeIdx);
    }

    static int modeIdx = static_cast<int>(mode_);
    const char* modeNames[] = {
        "LeftTop -> RightBottom",
        "RightBottom -> LeftTop",
        "RightTop -> LeftBottom",
        "LeftBottom -> RightTop",
        "Top -> Bottom",
        "Bottom -> Top",
        "Center -> Edges",
        "Edges -> Center"
    };
    if (ImGui::Combo("Transition Mode", &modeIdx, modeNames, IM_ARRAYSIZE(modeNames)))
    {
        mode_ = static_cast<TransitionMode>(modeIdx);
    }

    static int fadeIdx = static_cast<int>(fadeType_);
    const char* fadeNames[] = { "FadeOut", "FadeIn" };
    if (ImGui::Combo("Fade Type", &fadeIdx, fadeNames, IM_ARRAYSIZE(fadeNames)))
    {
        fadeType_ = static_cast<FadeType>(fadeIdx);
    }

    static float startCol[4]{ startColor_.x, startColor_.y, startColor_.z, startColor_.w };
    static float endCol[4]{ endColor_.x, endColor_.y, endColor_.z, endColor_.w };
    ImGui::ColorEdit4("Start Color", startCol);
    ImGui::ColorEdit4("End Color", endCol);
    if (ImGui::Button("Start Transition"))
    {
        Start(duration_, Vector4(startCol[0], startCol[1], startCol[2], 1.0f), Vector4(endCol[0], endCol[1], endCol[2], 1.0f));
    }
    if (state_ == TransitionState::Playing)
    {
        ImGui::ProgressBar(transitionRate_, ImVec2(0.0f, 0.0f));
        ImGui::Text("Rate: %.3f", transitionRate_);
    }
#endif
}
