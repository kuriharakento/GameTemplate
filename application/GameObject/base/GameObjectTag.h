#pragma once

/**
 * @file GameObjectTag.h
 * @brief ゲームオブジェクトのタグ定義
 * 
 * ゲーム内の全てのオブジェクトに付けられるタグの定数を定義します。
 * タグはオブジェクトの識別や衝突判定の分類に使用されます。
 */

namespace gameObjectTag
{
	/**
	 * @namespace Common
	 * @brief 共通のベースタグ定義
	 */
	namespace common {
		const std::string Character = "Character";
		const std::string CombatableObject = "CombatableObject";
		const std::string EnemyBase = "EnemyBase";
	}

	/**
	 * @namespace Character
	 * @brief キャラクター関連のタグ定義
	 */
	namespace character {
		// プレイヤー関連
		const std::string Player = "Player";
		const std::string PlayerRightArm = "PlayerRightArm";
		const std::string PlayerLeftArm = "PlayerLeftArm";

		// 敵キャラクター（銃器使用）
		const std::string AssaultEnemy = "AssaultEnemy";
		const std::string PistolEnemy = "PistolEnemy";
		const std::string ShotgunEnemy = "ShotgunEnemy";

		// 敵キャラクター（近接攻撃）
		const std::string KnifeEnemy = "KnifeEnemy";
		const std::string KnifeEnemyRightArm = "KnifeEnemyRightArm";
		const std::string KnifeEnemyLeftArm = "KnifeEnemyLeftArm";
	}

	/**
	 * @namespace Weapon
	 * @brief 武器関連のタグ定義
	 */
	namespace weapon {
		const std::string PlayerBullet = "PlayerBullet";
		const std::string EnemyBullet = "EnemyBullet";
		const std::string Knife = "Knife";
	}

	/**
	 * @namespace Item
	 * @brief アイテム・障害物関連のタグ定義
	 */
	namespace item {
		const std::string Obstacle = "Obstacle";
		const std::string BarrierBlock = "BarrierBlock";
		const std::string Floor = "Floor";
	}
}