#include "Area.h"

#include "application/gameObject/combatable/character/base/Character.h"
#include "engine/gameobject/component/collision/OBBColliderComponent.h"
#include "base/Logger.h"

using namespace GameObjectComponent;

Area::Area(Object3dCommon* objCommon, LightManager* lightManager, EnemyManager* enemyManager,
           const std::vector<Wave>& waves) : waveManager_(enemyManager, waves)
{
	// エリア境界を定義するゲームオブジェクトを作成
	areaObject_ = std::make_unique<GameObject>("Area");
	areaObject_->Initialize(objCommon, lightManager);

	// OBBコライダーを追加してエリア侵入検知を実装
	auto collider = std::make_unique<OBBColliderComponent>(areaObject_.get());

	// プレイヤーがエリアに侵入した時の処理
	collider->SetOnEnter([this](GameObject* other) {
		// プレイヤーがエリアに入ったら自動的にウェーブを開始
		if (!isStarted_ && other->GetTag() == gameObjectTag::character::Player)
		{
			Start();
		}
						 });

	// エリア内に留まっている間の処理（現在は未使用）
	collider->SetOnStay([this](GameObject* other) {
		// 必要に応じてエリア内の継続処理を実装
						});

	// エリアから出ようとした時の処理
	collider->SetOnExit([this](GameObject* other) {
		// キャラクターがエリア外に出ないようにする処理を実装可能
		auto character = dynamic_cast<Character*>(other);
		if (character)
		{
			// 必要に応じて境界制御を実装
		}

						});

	// コライダーをゲームオブジェクトに追加
	areaObject_->AddComponent("OBBCollider", std::move(collider));

	// エリアの初期状態を設定
	isStarted_ = false;  // 未開始
	isCleared_ = false;  // 未クリア
	isActive_ = false;   // 非アクティブ（AreaManagerによって制御される）
}

void Area::Start()
{
	// 冪等性の保証：既に開始されている場合は何もしない
	if (isStarted_) { return; }

	isStarted_ = true;
	isCleared_ = false;

	// WaveManagerに全ウェーブクリア時のコールバックを設定
	waveManager_.SetOnAllWavesCleared([this]() {
		isCleared_ = true;
		if (onClearCallback_)
		{
			// エリアクリア時のコールバックを実行（次エリアへの遷移など）
			onClearCallback_();
		}
									  });

	// ウェーブシーケンスを開始
	waveManager_.Start();
}

void Area::Update(CameraManager* camera)
{
	// エリア開始済みかつ未クリアの場合、ウェーブマネージャーを更新
	if (isStarted_ && !isCleared_)
	{
		waveManager_.Update();
	}

	// アクティブなエリアのみオブジェクトを更新
	if (isActive_)
	{
		// エリアオブジェクトの更新（コライダーなど）
		areaObject_->Update();
		areaObject_->UpdateTransform(camera);
	}
}

void Area::SetActive(bool active)
{
	// エリアのアクティブ状態を設定
	// 非アクティブなエリアはプレイヤー侵入を検知しない
	isActive_ = active;
}