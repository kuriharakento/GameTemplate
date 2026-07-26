#include "StatusComponent.h"
#include "engine/gameobject/base/GameObject.h"
#include "engine/time/TimeManager.h"
#include "engine/gameobject/component/base/ComponentFactory.h"

// ファクトリに登録
REGISTER_COMPONENT(StatusComponent)

namespace GameObjectComponent
{
	StatusComponent::StatusComponent(::GameObject* owner)
	{
		// メンバ変数をJSONエディタ/シリアライズ用に登録
		Register("hp", &hp_);
		Register("maxHp", &maxHp_);
		Register("isAlive", &isAlive_);
		Register("invincibleTimer", &invincibleTimer_);
		Register("isInvincibleMode", &isInvincibleMode_);
	}

	StatusComponent::~StatusComponent()
	{
	}

	void StatusComponent::Update(::GameObject* owner)
	{
		// 被弾後の無敵タイマーを減算
		if (invincibleTimer_ > 0.0f)
		{
			float dt = TimeManager::GetInstance().GetGameContext().deltaTime;
			invincibleTimer_ -= dt;
			if (invincibleTimer_ < 0.0f)
			{
				invincibleTimer_ = 0.0f;
			}
		}
	}

	bool StatusComponent::ApplyDamage(int32_t damage)
	{
		// デバッグ用無敵モード、または一時的無敵時間中、死亡中の場合はダメージを処理しない
		if (isInvincibleMode_ || invincibleTimer_ > 0.0f || !isAlive_)
		{
			return false;
		}

		hp_ -= damage;
		if (hp_ <= 0)
		{
			hp_ = 0;
			isAlive_ = false;
		}
		return true;
	}
}
