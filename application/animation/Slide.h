#pragma once
#include <memory>
#include <array>

#include "base/WinApp.h"
#include "engine/math/Easing.h"
#include "graphics/2d/Sprite.h"

class Slide
{
	public:
	Slide() = default;
	~Slide();

#ifdef USE_IMGUI
	void DrawImGui();
#endif
	/// \brief スライドの状態
	enum class Status
	{
		None,						//スライドなし
		SlideInFromLeft,			//左からスライドイン
		SlideOutFromLeft,			//左へスライドアウト
		SlideInFromBothSides,		//両サイドからスライドイン
		SlideOutFromBothSides,		//両サイドへスライドアウト
		SlideInFromFourCorners,     //四つ角からスライドイン
		SlideOutFromFourCorners,    //四つ角へスライドアウト
		SlideInWithRotation,        //回転しながらスライドイン
		SlideOutWithRotation,       //回転しながらスライドアウト
		SlideInDiagonal,            //斜め方向からスライドイン
		SlideOutDiagonal            //斜め方向へスライドアウト
	};

	/// \brief 初期化
	void Initialize(SpriteCommon* spriteCommon);
	/// \brief 更新
	void Update();
	/// \brief 描画
	void Draw();
	/// \brief スライド開始
	void Start(Status status, float duration);

public: //アクセッサ
	
	/**
	 * \brief イージング関数の設定
	 * \param pEasingFunc 
	 */
	void SetEasingFunc(float (*pEasingFunc)(float)) { pEasingFunc_ = pEasingFunc; }
	/**
	 * \brief イージング関数の取得
	 * \return 現在のイージング関数
	 */
	float (*GetEasingFunc() const)(float) { return pEasingFunc_; }
	/**
	 * \brief スライドが終了したかどうか
	 * \return 終了したらtrue
	 */
	bool IsFinish() const { return isFinish_; }

	/**
	 * \brief 経過時間取得
	 * \return 
	 */
	float GetCounter() const { return counter_; }

private: //メンバ関数

	/**
	 * \brief スプライトの初期化
	 */
	void InitializeSprites();
	/**
	 * \brief 演出終了後に呼び出される
	 */
	void Finish();
	/**
	 * \brief 両サイドからスプライトをスライドイン
	 * \param progress 進行具合
	 */
	void SlideInFromBothSides(float progress);
	/**
	 * \brief 両サイドからスライドアウト
	 * \param progress 
	 */
	void SlideOutFromBothSides(float progress);
	/**
	 * \brief 四つ角からスライドイン
	 * \param progress 
	 */
	void SlideInFromFourCorners(float progress);
	/**
	 * \brief 四つ角からスライドアウト
	 * \param progress 
	 */
	void SlideOutFromFourCorners(float progress);

	/**
	 * \brief 回転しながらスライドイン
	 * \param progress
	 */
	void SlideInWithRotation(float progress);
	/**
	 * \brief 回転しながらスライドアウト
	 * \param progress
	 */
	void SlideOutWithRotation(float progress);
	/**
	 * \brief 斜め方向からスライドイン
	 * \param progress
	 */
	void SlideInDiagonal(float progress);
	/**
	 * \brief 斜め方向へスライドアウト
	 * \param progress
	 */
	void SlideOutDiagonal(float progress);

private:

	/*----------------[ 構造体 ]------------------*/

	struct SlideSprite
	{
		std::unique_ptr<Sprite> sprite = nullptr;
		bool isMove = false;
	};

	// スライドの初期位置を表す構造体
	struct Direction {
		const float right;  // 右方向
		const float left;   // 左方向
		const float top;    // 上方向
		const float bottom; // 下方向

		// コンストラクタ
		Direction(float right, float left, float top, float bottom) : right(right), left(left), top(top), bottom(bottom) {}
	};

	/*----------------[ 変数 ]------------------*/

	/// \brief スライド用スプライトの複数バージョン
	std::array<SlideSprite, 4> sprites_;

	// \brief テクスチャハンドル
	const std::string kDebugPngPath = "./Resources/testSprite.png";
	const std::string kBlackPngPath = "./Resources/black.png";

	//現在のスライドの状態;
	Status status_ = Status::None;

	//終了
	bool isFinish_ = false;

	//スライドの持続時間
	float duration_ = 0.0f;

	//経過時間カウンター
	float counter_ = 0.0f;

	//イージング関数
	float (*pEasingFunc_)(float) = EaseInSine;

	//スライドの時間
	float easingTime_ = 1.0f;

	/*----------------[ 通常スライド用 ]------------------*/
	// 画面幅（WinAppから取得）
	const float kScreenWidth_ = static_cast<float>(WinApp::kClientWidth);
	// 画面高さ（WinAppから取得）
	const float kScreenHeight_ = static_cast<float>(WinApp::kClientHeight);
	// 画面幅の半分
	const float kHalfScreenWidth_ = kScreenWidth_ * 0.5f;
	// 画面高さの半分
	const float kHalfScreenHeight_ = kScreenHeight_ * 0.5f;

	// 移動量
	const float kSlideDistance_ = kScreenWidth_;

	// 画面イン時の初期位置
	const float kSlideInStartPos_ = -kScreenWidth_;
	// 画面アウト時の初期位置
	const float kSlideOutStartPos_ = 0.0f;

	/*----------------[ 両サイドスライド用 ]------------------*/

	// 両サイドからスライドイン時の初期位置
	Direction kSlideInBothSidesStartPos_ = Direction(kScreenWidth_, -kHalfScreenWidth_, 0.0f, 0.0f);

	// 両サイドからスライドアウト時の初期位置
	Direction kSlideOutBothSidesStartPos_ = Direction(kHalfScreenWidth_, 0.0f, 0.0f, 0.0f);

	// 両サイドからスライドするときの移動量
	const float kSlideBothSidesDistance_ = kHalfScreenWidth_;

	/*----------------[ 四つ角スライド用 ]------------------*/

	// 四つ角からスライドイン時のx初期位置
	Direction kSlideInFourCornersStartPos_ = Direction(kScreenWidth_, -kHalfScreenWidth_, -kHalfScreenHeight_, kScreenHeight_);

	// 四つ角からスライドアウト時の初期位置
	Direction kSlideOutFourCornersStartPos_ = Direction(kHalfScreenWidth_, 0.0f, 0.0f, kHalfScreenHeight_);

	// 四つ角からスライドするときの移動量
	const Vector2 kSlideFourCornersDistance_ = Vector2(kHalfScreenWidth_, kHalfScreenHeight_);

	/*----------------[ 回転スライド用 ]------------------*/

	// 回転スライドの中心
	const Vector2 kRotationCenter_ = Vector2(kHalfScreenWidth_, kHalfScreenHeight_);
};
