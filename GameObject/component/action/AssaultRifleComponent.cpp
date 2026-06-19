#include "AssaultRifleComponent.h"

// app
#include <engine/gameobject/base/GameObject.h>
#include "application/gameObject/combatable/character/player/Player.h"

#include "application/effect/BulletTrailManager.h"
#include "application/GameObject/component/action/StatusComponent.h"
// system
#include "graphics/3d/Object3dCommon.h"
#include "input/Input.h"
// component
#include "BulletComponent.h"
#include "engine/gameobject/component/collision/OBBColliderComponent.h"
#include "application/GameObject/component/action/MoveComponent.h"
// math
#include "math/MathUtils.h"
#include "time/TimeManager.h"
#include <audio/Audio.h>

namespace GameObjectComponent
{
	// コンストラクタ：武器の初期化
	AssaultRifleComponent::AssaultRifleComponent(::Object3dCommon* object3dCommon, ::LightManager* lightManager)
		: fireCooldown_(kDefaultFireCooldown), fireCooldownTimer_(0.0f)
	{
		object3dCommon_ = object3dCommon;
		lightManager_ = lightManager;
	}

	// デストラクタ：弾のクリーンアップ
	AssaultRifleComponent::~AssaultRifleComponent()
	{
		// 発射された弾をすべて削除（トレイルも解除）
		for (auto& bullet : bullets_)
		{
			// 先にトレイルを解除してからbulletを破棄
			BulletTrailManager::GetInstance().UnregisterBullet(bullet->GetTrailId());
			bullet.reset();
		}
		bullets_.clear();
	}

	// フレームごとの更新処理
	void AssaultRifleComponent::Update(::GameObject* owner)
	{
		// プレイヤーが所有している場合は realDeltaTime を使用してスローモーションを無視する
		float deltaTime;
		bool isPlayer = (dynamic_cast<::Player*>(owner) != nullptr);
		if (isPlayer)
		{
			deltaTime = TimeManager::GetInstance().GetGameContext().realDeltaTime;
		}
		else
		{
			deltaTime = TimeManager::GetInstance().GetGameContext().deltaTime;
		}

		// クールダウンタイマーを減少（ステータスの射撃レート倍率を適用）
		float fireRateMultiplier = 1.0f;
		if (auto status = owner->GetComponent<StatusComponent>())
		{
			fireRateMultiplier = status->fireRateMultiplier.GetValue();
		}
		fireCooldownTimer_ -= deltaTime * fireRateMultiplier;

		// リロード処理
		if (isReloading_)
		{
			Reload(deltaTime);
		}

		// プレイヤーの場合の入力処理
		if (auto player = dynamic_cast<::Player*>(owner))
		{
			// 選択中の武器のみ入力処理を行う
			if (IsActive())
			{
				// マウス左クリックで発射（クールダウン終了かつ弾があるかつリロードをしていない場合）
				if (Input::GetInstance()->IsMouseButtonPressed(0) && fireCooldownTimer_ <= 0.0f && currentAmmo_ > 0 && !isReloading_)
				{
					FireBullet(owner);
					fireCooldownTimer_ = fireCooldown_;
					
					// スローモーション中は弾薬を消費しない
					bool consumeAmmo = true;
					if (auto moveComp = owner->GetComponent<MoveComponent>())
					{
						if (moveComp->IsInBulletTime())
						{
							consumeAmmo = false;
						}
					}
					
					if (consumeAmmo)
					{
						currentAmmo_--;
						// 弾がなくなったらリロード開始
						if (currentAmmo_ <= 0)
						{
							StartReload();
						}
					}
				}

				// Rキーで手動リロード
				if (Input::GetInstance()->TriggerKey(DIK_R) && currentAmmo_ < maxAmmo_ && !isReloading_)
				{
					StartReload();
				}
			}
		}

		// すべての弾を更新
		for (const auto& bullet : bullets_)
			if (bullet->IsAlive()) bullet->Update();

		// 死んだ弾を削除（トレイルも解除）
		for (auto it = bullets_.begin(); it != bullets_.end();)
			if (!(*it)->IsAlive())
			{
				// トレイルを解除
				BulletTrailManager::GetInstance().UnregisterBullet((*it)->GetTrailId());
				it = bullets_.erase(it);
			}
			else ++it;
	}

	// 描画処理
	void AssaultRifleComponent::Draw3D(::CameraManager* camera)
	{
		// 弾のモデル描画は行わない（トレイルエフェクトで表現）
	}


	// プレイヤー用の弾発射処理
	void AssaultRifleComponent::FireBullet(::GameObject* owner)
	{
		// 弾の作成
		auto bullet = std::make_unique<::Bullet>(gameObjectTag::weapon::PlayerBullet);

		// カメラを取得
		::Camera* camera = object3dCommon_->GetDefaultCamera();
		if (!camera) return;

		// マウスのスクリーン座標を取得
		float mouseX = Input::GetInstance()->GetMouseX();
		float mouseY = Input::GetInstance()->GetMouseY();

		// ビューポート行列を作成（1280x720の仮想スクリーン解像度に固定）
		Matrix4x4 matViewport = MakeViewportMatrix(0, 0, 1280.0f, 720.0f, 0.0f, 1.0f);

		// ビュー行列とプロジェクション行列を合成
		Matrix4x4 matVPV = (camera->GetViewMatrix() * camera->GetProjectionMatrix()) * matViewport;

		// 合成行列の逆行列を計算
		Matrix4x4 matInverseVPV = Inverse(matVPV);

		// スクリーン座標を定義（近点と遠点）
		Vector3 posNear = Vector3(mouseX, mouseY, 0.0f);
		Vector3 posFar = Vector3(mouseX, mouseY, 1.0f);

		// スクリーン座標をワールド座標に変換
		posNear = MathUtils::Transform(posNear, matInverseVPV);
		posFar = MathUtils::Transform(posFar, matInverseVPV);

		// プレイヤーの位置を取得
		Vector3 playerPos = owner->GetPosition();

		// レイの方向を計算
		Vector3 rayDir = Vector3::Normalize(posFar - posNear);

		// プレイヤーと同じ高さの平面との交点を計算
		float t = (playerPos.y - posNear.y) / rayDir.y;
		Vector3 targetPos = posNear + rayDir * t;

		// 発射方向を計算
		Vector3 direction = Vector3::Normalize(targetPos - playerPos);

		// 水平方向の角度を計算
		Vector3 horizontalDir = Vector3(direction.x, 0.0f, direction.z);
		horizontalDir = Vector3::Normalize(horizontalDir);
		float rotationY = atan2f(horizontalDir.x, horizontalDir.z);

		// 弾の初期化（モデルは使わない）
		bullet->Initialize(object3dCommon_, lightManager_, playerPos);
		bullet->SetPosition(playerPos);
		bullet->SetRotation({ 0.0f, rotationY, 0.0f });
		bullet->SetScale(Vector3(kBulletScale, kBulletScale, 1.0f));

		// トレイルを登録
		uint32_t trailId = BulletTrailManager::GetInstance().RegisterBullet(bullet->GetTransform());
		bullet->SetTrailId(trailId);

		// BulletComponentを追加
		auto bulletComp = std::make_unique<BulletComponent>();
		bulletComp->Initialize(direction, speed_, lifetime_);
		bulletComp->SetIgnoreTimeScale(true);
		bullet->AddComponent("Bullet", std::move(bulletComp));

		// 衝突判定コンポーネントを追加
		auto colliderComp = std::make_unique<OBBColliderComponent>(bullet.get());
		colliderComp->SetOnEnter([ptr = bullet.get()](::GameObject* other) {
			// 敵に当たった場合、弾を消す
			if (other->GetTag() == gameObjectTag::character::PistolEnemy ||
				other->GetTag() == gameObjectTag::character::AssaultEnemy ||
				other->GetTag() == gameObjectTag::character::ShotgunEnemy ||
				other->GetTag() == gameObjectTag::character::KnifeEnemy
				)
			{
				ptr->SetAlive(false);
			}
			// 障害物に当たった場合、弾を消す
			if (other->GetTag() == gameObjectTag::item::Obstacle)
			{
				ptr->SetAlive(false);
			}
								 });
		bullet->AddComponent("OBBCollider", std::move(colliderComp));

		// 弾を管理リストに追加
		bullets_.push_back(std::move(bullet));

		// 効果音を再生
		::Audio::GetInstance()->PlayWave("fire_se", false);
	}


	// リロード開始
	void AssaultRifleComponent::StartReload()
	{
		isReloading_ = true;
		reloadTimer_ = 0.0f;
	}

	// リロード処理
	void AssaultRifleComponent::Reload(float deltaTime)
	{
		reloadTimer_ += deltaTime;

		// リロード完了
		if (reloadTimer_ >= reloadTime_)
		{
			currentAmmo_ = maxAmmo_;
			isReloading_ = false;
		}
	}
}

