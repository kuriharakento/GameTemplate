#include "AreaWaveData.h"

AreaWaveData::AreaWaveData()
{
	// JSON編集システムにareasメンバ変数を登録
	// これにより、JSONファイルとの自動連携が有効になる
	Register("areas", &areas);
}

void AreaWaveData::DrawImGui()
{
#ifdef USE_IMGUI
	// 各エリアを階層的に表示
    for (auto& area : areas)
    {
        ImGui::PushID(area.areaIndex);
        if (ImGui::CollapsingHeader(("Area " + std::to_string(area.areaIndex)).c_str()))
        {
            int waveIdx = 0;
			// エリア内の各ウェーブを表示
            for (auto& wave : area.waves)
            {
                ImGui::PushID(waveIdx);
                if (ImGui::TreeNode(("Wave " + std::to_string(waveIdx)).c_str()))
                {
                    int enemyIdx = 0;
					// ウェーブ内の各敵情報を表示
                    for (auto& enemy : wave.enemies)
                    {
                        ImGui::PushID(enemyIdx);
                        ImGui::Text("Enemy Name: %s", enemy.name.c_str());
						ImGui::Text("Type: %s", enemy.type.c_str());
						ImGui::Text("FileName: %s", enemy.fileName.c_str());
						// トランスフォーム情報の表示
						ImGui::Text("translate: (%.2f, %.2f, %.2f)", enemy.transform.translate.x, enemy.transform.translate.y, enemy.transform.translate.z);
						ImGui::Text("rotate: (%.2f, %.2f, %.2f)", enemy.transform.rotate.x, enemy.transform.rotate.y, enemy.transform.rotate.z);
						ImGui::Text("scale: (%.2f, %.2f, %.2f)", enemy.transform.scale.x, enemy.transform.scale.y, enemy.transform.scale.z);
                        ImGui::PopID();
                        enemyIdx++;
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
                waveIdx++;
            }
        }
        ImGui::PopID();
    }
#endif
}
