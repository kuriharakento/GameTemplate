#include "ObstacleData.h"
#ifdef USE_IMGUI
#include "imgui/imgui.h"
#endif

ObstacleData::ObstacleData()
{
	Register("objects", &obstacles);
}

void ObstacleData::Initialize(const std::string& name)
{
    LoadJson(name);
}

void ObstacleData::AddObstacle(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    
}

void ObstacleData::DrawImGui()
{
#ifdef USE_IMGUI
	// ImGuiでの編集UIを描画
	ImGui::SeparatorText("Obstacle Settings");

	ImGui::Text("Obstacle Count: %zu", obstacles.size());
	if(ImGui::CollapsingHeader("Obstacles"))
	{
		for (size_t i = 0; i < obstacles.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			ImGui::Text("Obstacle %zu", i + 1);
			ImGui::Text("Type: %s", obstacles[i].type.c_str());
			ImGui::Text("Name: %s", obstacles[i].name.c_str());
			ImGui::Checkbox("Disabled", &obstacles[i].disabled);
			DrawImGuiForTransform("Transform", &obstacles[i].transform);
			if (ImGui::Button("Delete"))
			{
				obstacles.erase(obstacles.begin() + i);
			}
			ImGui::PopID();
			if (i < obstacles.size() - 1)
			{
				ImGui::Separator();
			}
		}
		if (ImGui::Button("Add Obstacle"))
		{
			GameObjectInfo newObstacle;
			newObstacle.type = "DefaultType"; // デフォルトの種類
			newObstacle.name = "NewObstacle"; // デフォルトの名前
			newObstacle.transform.translate = Vector3(0.0f, 0.0f, 0.0f); // デフォルトの位置
			newObstacle.transform.rotate = Vector3(0.0f, 0.0f, 0.0f); // デフォルトの回転
			newObstacle.transform.scale = Vector3(1.0f, 1.0f, 1.0f); // デフォルトのスケール
			obstacles.push_back(newObstacle);
		}
	}
#endif // USE_IMGUI
}