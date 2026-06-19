#pragma once
#include <string>

#include "BTNode.h"
#include "CompositeNode.h"
#include "imgui/imgui.h"

/**
 * @namespace NodeUtils
 * @brief ビヘイビアツリーのノード操作用ユーティリティ関数群
 */
namespace nodeUtils
{
	/**
	 * @brief NodeStatusを文字列に変換
	 * @param status 変換するノードの状態
	 * @return 状態を表す文字列
	 */
	inline const std::string NodeStatusToString(NodeStatus status)
	{
		switch (status)
		{
		case NodeStatus::Success:
			return "Success";
		case NodeStatus::Failure:
			return "Failure";
		case NodeStatus::Running:
			return "Running";
		default:
			return "Unknown";
		}
	}

	/**
	 * @brief ビヘイビアツリーのノードをImGuiで再帰的に描画
	 * 
	 * ノードの状態に応じて色分けされたツリービューを表示します。
	 * デバッグ時のAI動作確認に使用します。
	 * 
	 * @param node 描画するノード
	 */
    inline void DrawBTNodeImGui(const BTNode* node)
    {
        if (!node) return;

        // ノード名 + アドレスによるユニークラベル
        std::string label = node->GetNodeName() + "##" + std::to_string(reinterpret_cast<uintptr_t>(node));

        // ノードの状態に応じて色を設定
        ImVec4 color;
        switch (node->GetLastStatus())
        {
        case NodeStatus::Running: color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); break;
        case NodeStatus::Success: color = ImVec4(0.2f, 0.6f, 1.0f, 1.0f); break;
        case NodeStatus::Failure: color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); break;
        default: color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        bool open = ImGui::TreeNode(label.c_str(), "%s [%s]", node->GetNodeName().c_str(), NodeStatusToString(node->GetLastStatus()).c_str());

        ImGui::PopStyleColor();

        if (open)
        {
            // 子ノードを再帰的に描画
            if (auto comp = dynamic_cast<const CompositeNode*>(node))
            {
                if (auto children = comp->GetChildren())
                {
                    for (const auto& child : *children)
                    {
                        DrawBTNodeImGui(child.get());
                    }
                }
            }
            ImGui::TreePop();
        }
    }
}
