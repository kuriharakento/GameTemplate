#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ecs/Registry.h"
#include "gameobject/base/GameObject.h"
#include "graphics/3d/InstancedModelRenderer.h"
#include "scene/interface/BaseScene.h"

/**
 * @brief 敵をたくさん出して描き方の速さを比べる計測用シーン
 *
 * - 同じ見た目・同じ動きの敵を 100/500/1000/2000 体出す
 * - 描く道を2つ切り替えられる
 *   1. GameObject 経路: GameObjectManager に登録して今まで通り描く
 *   2. ECS 経路: Registry と InstancedRenderSystem でまとめて描く
 * - 体数と道ごとの数字を測って、ボタンで crowd_bench.log へ書き出す
 */
class CrowdBenchScene : public KCE::BaseScene
{
public:
	void Initialize() override;
	void CommonUpdate() override;
	void Draw3D() override;
	void DrawGBuffer() override;
	void DrawShadow() override;
	void Draw2D() override;
	void DrawImGui() override;

protected:
	void OnFinalize() override;

private:
	/** @brief 描く道 */
	enum class Path
	{
		GameObject,
		Ecs,
	};

	/** @brief 今の体数と道に合わせて敵を作り直す */
	void Rebuild();
	/** @brief 敵を全部捨てる */
	void ClearCrowd();
	/** @brief GameObject 経路の敵を作る */
	void BuildGameObjects();
	/** @brief ECS 経路の敵を作る */
	void BuildEntities();
	/** @brief i 番目の敵の、その時刻の位置と Y 回転を決める（道によらず同じ式） */
	void GetCrowdTransform(int index, float time, KCE::Vector3& outPosition, float& outRotationY) const;
	/** @brief 今の平均と最大をファイルへ書き足す */
	void WriteRecord();

	// 選べる体数
	static constexpr int kCountChoices[] = { 100, 500, 1000, 2000 };
	// 並べる間隔と、1列に並べる数
	static constexpr float kSpacing = 2.2f;
	static constexpr int kPerRow = 50;
	// 敵の大きさ
	static constexpr float kEnemyScale = 0.5f;
	// 動きの大きさと速さ
	static constexpr float kMoveRadius = 0.8f;
	static constexpr float kMoveSpeed = 1.3f;
	static constexpr float kSpinSpeed = 0.9f;
	// 平均を取るフレーム数
	static constexpr int kSampleFrames = 120;
	// ECS の入れ物の大きさ
	static constexpr uint32_t kMaxEntities = 4096;
	// 使うモデル
	static constexpr const char* kEnemyModel = "cube";
	static constexpr const char* kFloorModel = "plane";
	static constexpr const char* kRecordPath = "crowd_bench.log";

	int count_ = 1000;
	Path path_ = Path::GameObject;
	// 次のフレームの頭で作り直す
	bool rebuildRequest_ = true;
	// 敵を動かすか（止めて描画だけ測りたいとき用）
	bool animate_ = true;
	float elapsed_ = 0.0f;

	// 床。GameObjectManager には登録せず、シーンが直接描く
	std::unique_ptr<KCE::Object3d> floor_;

	// GameObject 経路の敵。所有はこのシーン
	std::vector<std::unique_ptr<KCE::GameObject>> gameObjects_;

	// ECS 経路
	KCE::Registry registry_;
	bool registryReady_ = false;
	std::vector<KCE::EntityID> entities_;
	// モデル名ごとのまとめ描き。ECS 経路でだけ使う
	std::unordered_map<std::string, std::unique_ptr<KCE::InstancedModelRenderer>> ecsRenderers_;

	// フレーム時間の計測
	double frameMsTotal_ = 0.0;
	double frameMsMax_ = 0.0;
	int frameSamples_ = 0;
	double lastAverageMs_ = 0.0;
	double lastMaxMs_ = 0.0;
	// 直前の締めのときの描いた数・省いた数
	uint32_t lastDrawn_ = 0;
	uint32_t lastCulled_ = 0;
	// 直前の締めのときの、まとめて描いた数とまとまりの数
	uint32_t lastInstanced_ = 0;
	uint32_t lastGroups_ = 0;
};
