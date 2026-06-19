#include "StageData.h"

StageData::StageData()
{
	// JSON編集システムにgameObjectsメンバ変数を登録
	// これにより、JSONファイルとの自動連携が有効になる
	Register("objects", &gameObjects);
}

void StageData::DrawImGui()
{
#ifdef USE_IMGUI
	// 各ゲームオブジェクトの情報を表示・編集
	for (auto& obj : gameObjects)
	{
		ImGui::PushID(obj.name.c_str());
		ImGui::Text("Type: %s \nName: %s", obj.type.c_str(), obj.name.c_str());
		// 無効化フラグの編集
		ImGui::Checkbox("Disabled", &obj.disabled);
		// トランスフォームの編集
		ImGui::DragFloat3("Position", &obj.transform.translate.x, 0.1f);
		ImGui::DragFloat3("Rotation", &obj.transform.rotate.x, 0.1f);
		ImGui::DragFloat3("Scale", &obj.transform.scale.x, 0.1f);
		ImGui::PopID();
		ImGui::Separator();
	}
#endif
}
