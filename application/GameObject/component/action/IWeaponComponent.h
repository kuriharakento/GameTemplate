#pragma once
#include "engine/gameobject/component/base/IActionComponent.h"

class CameraManager;

/**
 * @brief 武器コンポーネントの共通インターフェース
 *
 * 全ての武器コンポーネントが実装すべき共通機能を定義。
 * WeaponManagerComponentで武器を統一的に扱うために使用する。
 */
namespace GameObjectComponent
{
	class IWeaponComponent : public IActionComponent
	{
	public:
		virtual ~IWeaponComponent() = default;

		/**
		 * @brief リロード開始
		 */
		virtual void StartReload() = 0;

		/**
		 * @brief 現在の弾数を取得
		 * @return 現在の弾数
		 */
		virtual int GetCurrentAmmo() const = 0;

		/**
		 * @brief 最大弾数を取得
		 * @return 最大弾数
		 */
		virtual int GetMaxAmmo() const = 0;

		/**
		 * @brief リロード中かどうか
		 * @return リロード中ならtrue
		 */
		virtual bool IsReloading() const = 0;

		/**
		 * @brief リロードの進行度を取得（0.0〜1.0）
		 * @return リロード進行度
		 */
		virtual float GetReloadProgress() const = 0;

		/**
		 * @brief 武器名を取得
		 * @return 武器の表示名
		 */
		virtual const char* GetWeaponName() const = 0;

		/**
		 * @brief 武器が有効（選択中）かどうか
		 * @return 有効ならtrue
		 */
		bool IsActive() const { return isActive_; }

		/**
		 * @brief 武器の有効状態を設定（WeaponManagerComponentから呼ばれる）
		 * @param active 有効にするならtrue
		 */
		void SetActive(bool active) { isActive_ = active; }

	protected:
		// 武器が選択中（有効）かどうか
		bool isActive_ = true;
	};
}

