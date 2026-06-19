#include "BehaviorTree.h"
#include "imgui/imgui.h"
#include <string>

void BehaviorTree::SetRoot(std::unique_ptr<BTNode> rootNode)
{
	root = std::move(rootNode);
}

void BehaviorTree::Tick()
{
    if (root)
    {
        root->Tick(blackboard);
    }
}

void BehaviorTree::Reset()
{
    if (root)
    {
        root->Reset();
    }
}

// ImGui用 再帰的描画関数
static void DrawNodeRecursive(const BTNode* node)
{
    if (!node) return;

    // 現在のステータスに応じた色設定
    ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
    NodeStatus status = node->GetLastStatus();
    std::string statusStr = "Unknown";
    switch (status)
    {
    case NodeStatus::Success: color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f); statusStr = "Success"; break; // Green
    case NodeStatus::Failure: color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); statusStr = "Failure"; break; // Red
    case NodeStatus::Running: color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f); statusStr = "Running"; break; // Yellow
    }

    // 表示用文字列
    std::string displayStr = node->GetNodeName() + " [" + statusStr + "]";

    // 子ノードの取得
    const auto* children = node->GetChildren();
    bool hasChildren = children && !children->empty();

    ImGui::PushStyleColor(ImGuiCol_Text, color);
    
    // ツリー要素として描画
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
    if (!hasChildren)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool isOpen = ImGui::TreeNodeEx((void*)node, flags, "%s", displayStr.c_str());
    ImGui::PopStyleColor();

    if (isOpen && hasChildren)
    {
        for (const auto& child : *children)
        {
            DrawNodeRecursive(child.get());
        }
        ImGui::TreePop();
    }
}

void BehaviorTree::DrawImGui()
{
#ifdef USE_IMGUI
    if (root)
    {
        DrawNodeRecursive(root.get());
    }
    else
    {
        ImGui::Text("BehaviorTree has no root node.");
    }
#endif
}
