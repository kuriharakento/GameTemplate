#include "Stage.h"

#include "graphics/3d/Object3dCommon.h"
#include "manager/editor/JsonEditor.h"
#include "engine/manager/graphics/LineManager.h"
#include "engine/math/VectorColorCodes.h"

Stage::Stage(Object3dCommon* object3dCommon, LightManager* lightManager, EnemyManager* ememyManager, const std::string& filePath)
{
	// エリア・ウェーブ情報をJSONファイルから読み込み
	areaWaveData_ = std::make_shared<AreaWaveData>();
	areaWaveData_->LoadJson(filePath);

	// デバッグエディターに登録（実行時編集を可能にする）
	JsonEditor::GetInstance()->Register("areaWaveData", areaWaveData_);

	// JSONデータをもとにAreaオブジェクトを生成
	std::vector<std::shared_ptr<Area>> areas;
	for (const auto& areaInfo : areaWaveData_->areas)
	{
		// エリア内の各Waveを生成
		std::vector<Wave> waves;
		for (const auto& waveInfo : areaInfo.waves)
		{
			std::vector<GameObjectInfo> enemies = waveInfo.enemies;
			waves.emplace_back(enemies);
		}

		// Areaオブジェクトを生成
		auto area = std::make_shared<Area>(object3dCommon, lightManager, ememyManager, waves);

		// 初期状態は非アクティブ（AreaManagerによって順次アクティブ化される）
		area->SetActive(false);

		// エリアの位置・回転・スケールを設定
		area->GetAreaObject()->SetPosition(areaInfo.areaTransform.translate);
		area->GetAreaObject()->SetRotation(areaInfo.areaTransform.rotate);
		area->GetAreaObject()->SetScale(areaInfo.areaTransform.scale);

		areas.push_back(area);
	}

	// AreaManagerを生成して全エリアを管理
	areaManager_ = std::make_unique<AreaManager>(areas);
	isCleared_ = false;
}

void Stage::Start()
{
    isCleared_ = false;

	// ステージクリア時のコールバック設定
	// AreaManagerが全エリアクリアを通知したらステージもクリアとする
    areaManager_->SetOnAllAreasCleared([this]() {
        isCleared_ = true;
        if (onClearCallback_) onClearCallback_();
                                       });

	// エリア進行を開始
    areaManager_->Start();
}

void Stage::Update(CameraManager* camera)
{
	// ステージクリア済みなら更新不要
    if (!isCleared_)
    {
#ifdef _DEBUG
		// デバッグモード：敵のスポーン位置を可視化
		// JSONで定義された各エリアの各ウェーブの敵スポーン位置を表示
        for (const auto& areaInfo : areaWaveData_->areas)
        {
            // エリア原点（ワールド座標）
            Vector3 areaOrigin = areaInfo.areaTransform.translate;
            for (const auto& waveInfo : areaInfo.waves)
            {
                for (const auto& enemyInfo : waveInfo.enemies)
                {
					// 敵の位置にキューブを描画して視覚化
                    LineManager::GetInstance()->DrawCube(
                        enemyInfo.transform.translate,
                        1.0f,
                        VectorColorCodes::Purple
                    );
                }
            }
        }
#endif
		// AreaManagerの更新
        areaManager_->Update(camera);
    }
}