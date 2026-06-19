#include "AreaManager.h"
#include <effects/particle/ParticleEffect.h>
#include <effects/particle/module/spawn/InitialModules.h>

AreaManager::AreaManager(const std::vector<std::shared_ptr<Area>>& areas) : areas_(areas), currentAreaIndex_(0), isAllCleared_(false)
{
	// パーティクルをJsonから読み込み
	areaEffect_ = ParticleManager::GetInstance()->Load("AreaEffect", "Resources/json/particle/zone.json");
	if (areaEffect_)
	{
		areaEffect_->SetAutoRemove(false); // 自動削除を無効化
	}
}

void AreaManager::Start()
{
	// エリアが存在しない場合は早期リターン
	if (areas_.empty()) { return; };

	// 最初のエリアを開始
	StartCurrentArea();
}

void AreaManager::Update(CameraManager* camera)
{
	// 全エリアクリア済みの場合は更新不要
	if (isAllCleared_) { return; }

	// 現在のエリアを更新
	if (currentAreaIndex_ < areas_.size())
	{
		areas_[currentAreaIndex_]->Update(camera);
	}
}

void AreaManager::StartCurrentArea()
{
	// 全エリア終了チェック
	if (currentAreaIndex_ >= areas_.size())
	{
		// 全エリアクリア状態に設定
		isAllCleared_ = true;

		// エリアエフェクトを停止
		if (areaEffect_)
		{
			areaEffect_->Stop();
		}

		// 全エリアクリアコールバックを実行
		if (onAllAreasCleared_) onAllAreasCleared_();
		return;
	}

	// 現在のエリアにクリアコールバックを設定
	// エリアクリア時に次のエリアへ進む
	areas_[currentAreaIndex_]->SetOnClearCallback([this]() {
		++currentAreaIndex_;
		StartCurrentArea();
												  });

	// エリアをアクティブ化（プレイヤー侵入検知を有効化）
	areas_[currentAreaIndex_]->SetActive(true);

	// エリアエフェクトの位置設定と再生
	areaEffect_->SetPosition(areas_[currentAreaIndex_]->GetAreaObject()->GetPosition());
	// 初期スケールモジュールを取得して、エリアに合わせて調整
	if (auto* emitter = areaEffect_->GetEmitter(static_cast<size_t>(0)))
	{
		if (auto* scale = emitter->GetModule<InitialScaleModule>())
		{
			// エリアのスケールを反映
			scale->SetMaxScale({ areas_[currentAreaIndex_]->GetAreaObject()->GetScale().x, 0.4f, areas_[currentAreaIndex_]->GetAreaObject()->GetScale().z });
			scale->SetMinScale({ areas_[currentAreaIndex_]->GetAreaObject()->GetScale().x, 0.4f, areas_[currentAreaIndex_]->GetAreaObject()->GetScale().z });
		}
	}

	areaEffect_->Play();

	// エリア開始コールバックを実行（演出制御用）
	if (onAreaStarted_)
	{
		onAreaStarted_(currentAreaIndex_, areas_[currentAreaIndex_].get());
	}

	// 前のエリアを非アクティブ化
	if (currentAreaIndex_ > 0)
	{
		areas_[currentAreaIndex_ - 1]->SetActive(false);
	}
}

