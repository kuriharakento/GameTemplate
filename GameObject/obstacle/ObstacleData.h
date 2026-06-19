#pragma once
#include "application/stage/GameObjectInfo.h"
#include "base/GraphicsTypes.h"
#include "externals/nlohmann/json.hpp"
#include "jsonEditor/JsonEditableBase.h"
#include "math/Vector3.h"

using json = nlohmann::json;

class ObstacleData : public JsonEditableBase
{
public:
    ObstacleData();
    void Initialize(const std::string& name);
    void AddObstacle(const Vector3& position, const Vector3& rotation, const Vector3& scale);
    void DrawImGui() override;
	void SetObstacles(const std::vector<GameObjectInfo>& obs) { obstacles = obs; } // 障害物のTransform情報を設定
	std::vector<GameObjectInfo>& GetObstacles() { return obstacles; } // 障害物のTransform情報を取得
	uint32_t GetObstacleCount() const { return static_cast<uint32_t>(obstacles.size()); } // 障害物の数を取得

private:
	std::vector<GameObjectInfo> obstacles; // 障害物のTransform情報
};