#pragma once
#include "application/GameObject/combatable/base/CombatableObject.h"

/**
 * @brief 近接武器（ナイフ）を表すクラス
 * 
 * 敵キャラクターが装備する近接攻撃武器です。
 * 親オブジェクト（キャラクターの腕）の子オブジェクトとして配置されます。
 */
class Knife : public CombatableObject
{
public:
	~Knife() = default;
	
	/**
	 * @brief コンストラクタ
	 * @param tag ナイフのタグ
	 */
	Knife(const std::string& tag) : CombatableObject(tag) {}

	/**
	 * @brief ナイフの初期化
	 * @param object3dCommon 3Dオブジェクト共通データ
	 * @param lightManager ライト管理クラス
	 */
	void Initialize(Object3dCommon* object3dCommon, LightManager* lightManager);
	
	/**
	 * @brief 描画処理
	 * @param camera カメラ管理クラス
	 */
	void Draw(CameraManager* camera);
};

