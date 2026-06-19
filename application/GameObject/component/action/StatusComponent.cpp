#include "StatusComponent.h"

#include "engine/gameobject/base/GameObject.h"
#ifdef USE_IMGUI
#include "imgui/imgui.h"
#include "manager/editor/DebugUIManager.h"
#endif
#include "time/TimeManager.h"
#include "manager/editor/JsonEditor.h"

// Factory
#include "engine/gameobject/component/base/ComponentFactory.h"
REGISTER_COMPONENT(StatusComponent)

namespace GameObjectComponent
{
	StatusComponent::StatusComponent()
	{
		REGISTER_MEMBER(hp);
		REGISTER_MEMBER(maxHp);
		REGISTER_MEMBER(attackPower);
		REGISTER_MEMBER(moveSpeed);
		REGISTER_MEMBER(fireRateMultiplier);
		REGISTER_MEMBER(isAlive);

	#ifdef USE_IMGUI
		DebugUIManager::GetInstance()->RegisterDebugUI(this, "Status Component", [this]() { this->DrawImGui(); }, DebugUIArea::Inspector);
	#endif
	}

	StatusComponent::~StatusComponent()
	{
		JsonEditor::GetInstance()->Unregister(this);
	#ifdef USE_IMGUI
		if (DebugUIManager::HasInstance()) {
			DebugUIManager::GetInstance()->UnregisterDebugUI(this);
		}
	#endif
	}

	void StatusComponent::DrawImGui()
	{
	#ifdef USE_IMGUI
		if (lastOwner_)
		{
			std::string headerTitle = "Status: " + lastOwner_->GetTag();
			ImGui::SeparatorText(headerTitle.c_str());
		}
		// 自動編集UIの描画
		JsonEditableBase::DrawImGui();
	#endif
	}

	// フレームごとの更新処理
	void StatusComponent::Update(::GameObject* owner)
	{
		lastOwner_ = owner;
		// デルタタイムを取得
		float deltaTime = TimeManager::GetInstance().GetGameContext().deltaTime;

		// 各ステータス値を更新
		hp.Update(deltaTime);
		maxHp.Update(deltaTime);
		attackPower.Update(deltaTime);
		moveSpeed.Update(deltaTime);
		fireRateMultiplier.Update(deltaTime);

		// HPが0以下になったら死亡判定
		if (hp.GetValue() <= kDeathThreshold)
		{
			isAlive = false;
		}
	}
}

