#pragma once

#include "StatusSystem.h"
#include "engine/gameObject/base/GameObject.h"
#include "application/gameObject/component/action/StatusComponent.h"
#include "application/gameObject/base/GameObjectTag.h"

/**
 * @brief 戦闘可能なオブジェクトの基底クラス
 * 
 * HP、攻撃力などのステータスを持ち、ダメージを受けることができます。
 * StatusComponentを自動的に追加し、ステータス管理機能を提供します。
 */
class CombatableObject : public GameObject
{
public:
    virtual ~CombatableObject() = default;
    
    /**
     * @brief コンストラクタ
     * @param tag オブジェクトのタグ
     */
    explicit CombatableObject(const std::string& tag = gameObjectTag::common::CombatableObject)
        : GameObject(tag)
    {
		AddComponent("StatusComponent", std::make_unique<GameObjectComponent::StatusComponent>());
    }

    /**
     * @brief ダメージを受ける
     * @param damage 受けるダメージ量
     */
	virtual void TakeDamage(float damage)
	{
		auto status = GetComponent<GameObjectComponent::StatusComponent>();
		if (status && status->isAlive)
		{
			float newHp = status->hp.GetValue() - damage;
			status->hp.SetBase(newHp);
			if (newHp <= 0.0f)
			{
				status->isAlive = false;
				status->hp.SetBase(0.0f);
			}
		}
	}

	//======================================
	// ステータスのGetter/Setter
	//======================================

    /**
     * @brief HPの取得
     * @return 現在のHP値
     */
    float GetHp() const
    {
        auto status = GetComponent<GameObjectComponent::StatusComponent>();
        return status ? status->hp.GetValue() : 0.0f;
    }
    
    /**
     * @brief HPの設定
     * @param v 設定するHP値
     */
    void SetHp(float v)
    {
        auto status = GetComponent<GameObjectComponent::StatusComponent>();
        if (status)
        {
            status->hp.SetBase(v);
        }
    }

    /**
     * @brief 攻撃力の取得
     * @return 現在の攻撃力
     */
    float GetAttackPower() const
    {
        auto status = GetComponent<GameObjectComponent::StatusComponent>();
        return status ? status->attackPower.GetValue() : 0.0f;
    }
    
    /**
     * @brief 攻撃力の設定
     * @param v 設定する攻撃力
     */
    void SetAttackPower(float v)
    {
        auto status = GetComponent<GameObjectComponent::StatusComponent>();
        if (status)
        {
            status->attackPower.SetBase(v);
        }
    }

    /**
     * @brief 生存状態の取得
     * @return 生存している場合true
     */
    bool IsAlive() const
    {
        auto status = GetComponent<GameObjectComponent::StatusComponent>();
        return status ? status->isAlive : false;
    }
    
    /**
     * @brief 生存状態の設定
     * @param alive 設定する生存状態
     */
    void SetAlive(bool alive)
    {
        auto status = GetComponent<GameObjectComponent::StatusComponent>();
        if (status)
        {
            status->isAlive = alive;
        }
    }
};
