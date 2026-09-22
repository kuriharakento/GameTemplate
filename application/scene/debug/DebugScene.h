#pragma once
#include "scene/interface/BaseScene.h"

/**
 * @brief 実装したものをその場で試すための空のシーン。
 *
 * - 確認が終わったら、足したものは消して空に戻しておく
 */
class DebugScene : public KCE::BaseScene
{
public:
	void Initialize() override;
	void CommonUpdate() override;
	void Draw3D() override;
	void Draw2D() override;

protected:
	void OnFinalize() override;
};
