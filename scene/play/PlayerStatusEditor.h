#pragma once
#include "jsonEditor/JsonEditableBase.h"
#include "engine/ecs/Registry.h"
#include "application/ecs/components/StatusComponent.h"
#include "manager/editor/JsonEditor.h"

/**
 * @brief プレイヤーのECSステータスをJSON編集するためのプロキシクラス
 */
class PlayerStatusEditor : public JsonEditableBase
{
public:
	PlayerStatusEditor(Registry* registry, EntityID playerEntity)
		: registry_(registry), playerEntity_(playerEntity)
	{
		REGISTER_MEMBER(hp);
		REGISTER_MEMBER(maxHp);
		REGISTER_MEMBER(attackPower);
		REGISTER_MEMBER(moveSpeed);

		SetFileName("PlayerStatus.json");
	}

	~PlayerStatusEditor() override
	{
		JsonEditor::GetInstance()->Unregister(this);
	}

	/**
	 * @brief ECSのStatusComponentと同期する
	 */
	void Sync()
	{
		if (!registry_ || playerEntity_ == kInvalidEntity) return;
		if (!registry_->HasComponent<ecs::StatusComponent>(playerEntity_)) return;

		auto& ecsStatus = registry_->GetComponent<ecs::StatusComponent>(playerEntity_);

		// エディタ側での編集を優先してECSに書き戻す、またはゲーム内の変動をエディタに反映する
		if (lastHp_ != hp)
		{
			ecsStatus.hp_.SetBase(hp);
			lastHp_ = hp;
		}
		else
		{
			hp = ecsStatus.hp_.GetValue();
			lastHp_ = hp;
		}

		if (lastMaxHp_ != maxHp)
		{
			ecsStatus.maxHp_.SetBase(maxHp);
			lastMaxHp_ = maxHp;
		}
		else
		{
			maxHp = ecsStatus.maxHp_.GetValue();
			lastMaxHp_ = maxHp;
		}

		if (lastAttackPower_ != attackPower)
		{
			ecsStatus.attackPower_.SetBase(attackPower);
			lastAttackPower_ = attackPower;
		}
		else
		{
			attackPower = ecsStatus.attackPower_.GetValue();
			lastAttackPower_ = attackPower;
		}

		if (lastMoveSpeed_ != moveSpeed)
		{
			ecsStatus.moveSpeed_.SetBase(moveSpeed);
			lastMoveSpeed_ = moveSpeed;
		}
		else
		{
			moveSpeed = ecsStatus.moveSpeed_.GetValue();
			lastMoveSpeed_ = moveSpeed;
		}
	}

	/**
	 * @brief JSONロード後にECSへ一括反映する
	 */
	void ApplyToECS()
	{
		if (!registry_ || playerEntity_ == kInvalidEntity) return;
		if (!registry_->HasComponent<ecs::StatusComponent>(playerEntity_)) return;

		auto& ecsStatus = registry_->GetComponent<ecs::StatusComponent>(playerEntity_);
		ecsStatus.hp_.SetBase(hp);
		ecsStatus.maxHp_.SetBase(maxHp);
		ecsStatus.attackPower_.SetBase(attackPower);
		ecsStatus.moveSpeed_.SetBase(moveSpeed);

		lastHp_ = hp;
		lastMaxHp_ = maxHp;
		lastAttackPower_ = attackPower;
		lastMoveSpeed_ = moveSpeed;
	}

public:
	float hp = 100.0f;
	float maxHp = 100.0f;
	float attackPower = 10.0f;
	float moveSpeed = 5.0f;

private:
	Registry* registry_ = nullptr;
	EntityID playerEntity_ = kInvalidEntity;

	float lastHp_ = 100.0f;
	float lastMaxHp_ = 100.0f;
	float lastAttackPower_ = 10.0f;
	float lastMoveSpeed_ = 5.0f;
};
