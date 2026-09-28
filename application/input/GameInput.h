#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include "math/Vector2.h"

namespace KCE
{
/**
 * @brief ゲーム側で使う操作の一覧。
 *
 * - デバイスに依存しない名前にしておく。実際のキーやボタンは GameInput のバインド表で決める
 * - 増やすときは Count の前に足す
 */
enum class GameAction
{
	MoveForward,
	MoveBack,
	MoveLeft,
	MoveRight,
	Jump,
	Attack,
	Pause,
	Count
};

/**
 * @brief 入力をロックする理由。
 *
 * 理由ごとに別々にロックを持つので、どれか1つでも残っていればロック中になる。
 * 増やすときは Count の前に足す。
 */
enum class InputLockReason
{
	Intro, // スタート演出
	Pause, // ポーズメニュー
	DebugCamera, // デバッグカメラで飛び回っている間
	Count
};

/**
 * @brief アクションに割り当てる1つの入力。
 */
struct InputBinding
{
	enum class Device
	{
		Keyboard,           // code は DIK_*
		Gamepad,            // code は XINPUT_GAMEPAD_*
		Mouse,              // code はボタン番号（0:左、1:中、2:右）
		GamepadStickPlus,   // code は GamepadAxis。スティックがプラス方向へ倒された時に押下扱い
		GamepadStickMinus   // code は GamepadAxis。スティックがマイナス方向へ倒された時に押下扱い
	};

	// GamepadStickPlus/Minus で使うスティック軸
	enum class GamepadAxis : uint32_t
	{
		LeftStickX,
		LeftStickY,
		RightStickX,
		RightStickY
	};

	Device device;
	uint32_t code;
};

/**
 * @brief ゲームのアクションを実際のキーやボタンに変換する（シングルトン）。
 *
 * - デバイスの読み取りは KCE::Input に任せる。KCE::Input::Update の後に Update を1回呼ぶ前提
 * - 1つのアクションに複数の入力を割り当てられる。どれか押されていれば押している扱い
 * - ロック中はアクションが全部「押されていない」になる。ロックを無視するアクションだけは通す
 * - ゲームパッドは0番だけを見る
 */
class GameInput
{
public:
	/**
	 * @brief インスタンスを取得する。
	 * @return GameInput のポインタ（所有しない）
	 */
	static GameInput* GetInstance();

	/**
	 * @brief 既定のバインドを設定して、状態をリセットする。
	 */
	void Initialize();

	/**
	 * @brief 終了処理。デバッグ表示の登録を外す。
	 */
	void Finalize();

	/**
	 * @brief アクションの状態を1フレーム分更新する。
	 */
	void Update();

	/**
	 * @brief Settings の「デバッグ > ゲーム入力」に出す確認用の表示。
	 */
	void DrawImGui();

	/**
	 * @brief アクションに入力を追加で割り当てる。
	 * @param action 割り当て先
	 * @param binding 追加する入力
	 */
	void AddBinding(GameAction action, const InputBinding& binding);

	/**
	 * @brief アクションの割り当てを全部外す。付け直すときに使う。
	 * @param action 外すアクション
	 */
	void ClearBindings(GameAction action);

	/**
	 * @brief アクションがロックを無視するか設定する。ポーズなど、ロック中も受け付けたい操作用。
	 * @param action 対象のアクション
	 * @param ignore 無視するなら true
	 */
	void SetIgnoreLock(GameAction action, bool ignore);

	/**
	 * @brief 押しているか。
	 * @param action 調べるアクション
	 * @return 押しているなら true
	 */
	bool IsPressed(GameAction action) const;

	/**
	 * @brief 押した瞬間か。
	 * @param action 調べるアクション
	 * @return 押した瞬間なら true
	 */
	bool IsTriggered(GameAction action) const;

	/**
	 * @brief 離した瞬間か。ロックが掛かった瞬間も、押していたなら離した扱いになる。
	 * @param action 調べるアクション
	 * @return 離した瞬間なら true
	 */
	bool IsReleased(GameAction action) const;

	/**
	 * @brief 指定の理由でロックする。同じ理由で何回呼んでも1回分の扱い。
	 * @param reason ロックする理由
	 */
	void Lock(InputLockReason reason);

	/**
	 * @brief 指定の理由のロックを外す。他の理由が残っていればロックは続く。
	 * @param reason 外す理由
	 */
	void Unlock(InputLockReason reason);

	/**
	 * @brief 移動方向をアナログで取得する。
	 *
	 * 左スティックの入力をそのまま返し、スティックが無入力（デッドゾーン内）のときは
	 * MoveForward/Back/Left/Right のデジタル入力から (-1,0,1) の値を組み立てて返す。
	 * ロック中は (0,0) になる。
	 * @return x: 右方向プラス、y: 前方向プラス。斜め入力は正規化される
	 */
	KCE::Vector2 GetMoveVector() const;

	/**
	 * @brief ゲームパッドの振動を設定する。
	 * @param leftMotor 低周波（左）モーターの強さ（0〜65535）
	 * @param rightMotor 高周波（右）モーターの強さ（0〜65535）
	 */
	void SetVibration(uint16_t leftMotor, uint16_t rightMotor);

	/**
	 * @brief ゲームパッドの振動を止める。
	 */
	void StopVibration();

	/**
	 * @brief どれかの理由でロックされているか。
	 * @return ロック中なら true
	 */
	bool IsLocked() const { return lockMask_ != 0; }

	/**
	 * @brief 指定の理由でロックされているか。
	 * @param reason 調べる理由
	 * @return その理由でロック中なら true
	 */
	bool IsLockedBy(InputLockReason reason) const;

private:
	friend std::unique_ptr<GameInput> std::make_unique<GameInput>();
	GameInput() = default;

	// コピーと代入を禁止
	GameInput(const GameInput&) = delete;
	GameInput& operator=(const GameInput&) = delete;

public:
	~GameInput();

private:
	static constexpr size_t kActionCount = static_cast<size_t>(GameAction::Count);
	static_assert(static_cast<size_t>(InputLockReason::Count) <= 32, "lockMask_ に入りきらない");

	/** @brief 既定のキーとボタンを割り当てる */
	void SetDefaultBindings();

	/** @brief 割り当てた入力のどれかが押されているか。ロックは見ない */
	bool ReadRawPressed(GameAction action) const;

	/** @brief このアクションが今ロックで止められているか */
	bool IsBlocked(GameAction action) const;

	static std::unique_ptr<GameInput> instance_;

	// アクションごとの割り当て。増えるのは初期化時だけ
	std::array<std::vector<InputBinding>, kActionCount> bindings_{};
	// ロックを無視するアクション
	std::array<bool, kActionCount> ignoreLock_{};

	// ロック込みの今回と前回の状態
	std::array<bool, kActionCount> current_{};
	std::array<bool, kActionCount> previous_{};
	// 前フレームにロックで止められていたか。解除した瞬間の誤判定を防ぐのに使う
	std::array<bool, kActionCount> wasBlocked_{};

	// InputLockReason ごとのビット
	uint32_t lockMask_ = 0;

	// デバッグ表示用。Triggered / Released は1フレームしか立たないので回数で見る
	std::array<int, kActionCount> debugTriggerCounts_{};
	std::array<int, kActionCount> debugReleaseCounts_{};
};
} // namespace KCE
