#include "ShotgunComponent.h"

// system
#include "graphics/3d/Object3dCommon.h"
#include "input/Input.h"
// app
#include <engine/gameobject/base/GameObject.h>
#include "application/gameObject/combatable/character/player/Player.h"
#include "application/GameObject/component/action/StatusComponent.h"
#include "application/GameObject/component/action/MoveComponent.h"
// component
#include "BulletComponent.h"
#include "engine/gameobject/component/collision/OBBColliderComponent.h"
// math
#include "math/MathUtils.h"
#include <random>

#include "time/TimeManager.h"

namespace GameObjectComponent
{
    // コンストラクタ：武器の初期化
    ShotgunComponent::ShotgunComponent(Object3dCommon* object3dCommon, LightManager* lightManager)
        : fireCooldown_(kDefaultFireCooldown), fireCooldownTimer_(0.0f)
    {
        object3dCommon_ = object3dCommon;
        lightManager_ = lightManager;
    }

    // デストラクタ：弾のクリーンアップ
    ShotgunComponent::~ShotgunComponent()
    {
        // 発射された弾をすべて削除
        for (auto& bullet : bullets_) bullet.reset();
        bullets_.clear();
    }

    // フレームごとの更新処理
    void ShotgunComponent::Update(::GameObject* owner)
    {
        // プレイヤーが所有している場合は realDeltaTime を使用してスローモーションを無視する
        bool isPlayerOwner = dynamic_cast<::Player*>(owner) != nullptr;
        float deltaTime = isPlayerOwner ? TimeManager::GetInstance().GetGameContext().realDeltaTime : TimeManager::GetInstance().GetGameContext().deltaTime;

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

        if (!isReloading_)
        {
            // プレイヤーの場合の入力処理
            if (auto player = dynamic_cast<::Player*>(owner))
            {
                // 選択中の武器のみ入力処理を行う
                if (IsActive())
                {
                    // マウス左クリックで発射
                    if (Input::GetInstance()->IsMouseButtonTriggered(0) && fireCooldownTimer_ <= 0.0f && currentAmmo_ > 0)
                    {
                        FireBullets(owner);
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
                            if (currentAmmo_ <= 0) StartReload();
                        }
                    }
                    // Rキーで手動リロード
                    if (Input::GetInstance()->TriggerKey(DIK_R) && currentAmmo_ < maxAmmo_)
                    {
                        StartReload();
                    }
                }
            }

        }

        // すべての弾を更新
        for (const auto& bullet : bullets_)
            if (bullet->IsAlive()) bullet->Update();

        // 死んだ弾を削除
        for (auto it = bullets_.begin(); it != bullets_.end();)
            if (!(*it)->IsAlive()) it = bullets_.erase(it);
            else ++it;
    }

    // 描画処理
    void ShotgunComponent::Draw3D(CameraManager* camera)
    {
        // 生存している弾のみ描画
        for (const auto& bullet : bullets_)
            if (bullet->IsAlive()) bullet->Draw3D(camera);
    }


    // プレイヤー用の弾発射処理
    void ShotgunComponent::FireBullets(::GameObject* owner)
    {
        // カメラを取得
        Camera* camera = object3dCommon_->GetDefaultCamera();
        if (!camera) return;

        // マウスのスクリーン座標を取得
        float mouseX = Input::GetInstance()->GetMouseX();
        float mouseY = Input::GetInstance()->GetMouseY();

        // ビビューポート行列を作成（1280x720の仮想スクリーン解像度に固定）
        Matrix4x4 matViewport = MakeViewportMatrix(0, 0, 1280.0f, 720.0f, 0.0f, 1.0f);

        // ビュー行列とプロジェクション行列を合成
        Matrix4x4 matVPV = (camera->GetViewMatrix() * camera->GetProjectionMatrix()) * matViewport;
        Matrix4x4 matInverseVPV = Inverse(matVPV);

        // スクリーン座標をワールド座標に変換
        Vector3 posNear = Vector3(mouseX, mouseY, 0.0f);
        Vector3 posFar = Vector3(mouseX, mouseY, 1.0f);
        posNear = MathUtils::Transform(posNear, matInverseVPV);
        posFar = MathUtils::Transform(posFar, matInverseVPV);

        // レイの方向を計算
        Vector3 rayDir = Vector3::Normalize(posFar - posNear);
        Vector3 playerPos = owner->GetPosition();

        // 地面との交点を計算
        float t = -posNear.y / rayDir.y;
        Vector3 targetPos = posNear + rayDir * t;

        // 基準方向を計算
        Vector3 baseDir = Vector3::Normalize(targetPos - playerPos);
        baseDir.y = 0.0f;

        float baseAngle = atan2f(baseDir.x, baseDir.z);

        // 乱数生成器の用意
        static std::random_device rd;
        static std::mt19937 gen(rd());
        // ばらけ角度の範囲
        std::uniform_real_distribution<float> angleDist(-spreadAngle_, spreadAngle_);
        // Y方向の微小ばらけ
        std::uniform_real_distribution<float> yDist(-kVerticalSpreadRange, kVerticalSpreadRange);

        // 複数の弾を発射
        for (int i = 0; i < pelletCount_; ++i)
        {
            // ランダムな角度オフセットを計算
            float angleOffset = angleDist(gen) * kDegToRad;
            float angle = baseAngle + angleOffset;
            float yOffset = yDist(gen);

            // 発射方向を計算
            Vector3 dir = { sinf(angle), yOffset, cosf(angle) };
            dir = Vector3::Normalize(dir);

            // 弾の作成と初期化
            auto bullet = std::make_unique<::Bullet>(gameObjectTag::weapon::PlayerBullet);
            bullet->Initialize(object3dCommon_, lightManager_, playerPos);
            bullet->SetModel("cube.obj");
            bullet->SetRotation({ 0.0f, angle, 0.0f });
            bullet->SetScale(Vector3(kBulletScale, kBulletScale, 1.0f));

            // BulletComponentを追加
            auto bulletComp = std::make_unique<BulletComponent>();
            bulletComp->Initialize(dir, kBulletSpeed, kBulletLifetime);
            bulletComp->SetIgnoreTimeScale(true);
            bullet->AddComponent("Bullet", std::move(bulletComp));

            // 衝突判定コンポーネントを追加
            auto colliderComp = std::make_unique<OBBColliderComponent>(bullet.get());
            colliderComp->SetOnEnter([ptr = bullet.get()](::GameObject* other) {
                // 敵に当たった場合、弾を消す
                if (other->GetTag() == gameObjectTag::character::PistolEnemy || other->GetTag() == gameObjectTag::character::AssaultEnemy || other->GetTag() == gameObjectTag::character::ShotgunEnemy)
                    ptr->SetAlive(false);
                                     });
            bullet->AddComponent("OBBCollider", std::move(colliderComp));

            // 弾を管理リストに追加
            bullets_.push_back(std::move(bullet));
        }
    }


    // リロード開始
    void ShotgunComponent::StartReload()
    {
        isReloading_ = true;
        reloadTimer_ = 0.0f;
    }

    // リロード処理
    void ShotgunComponent::Reload(float deltaTime)
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
