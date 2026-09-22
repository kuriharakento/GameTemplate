#include "GameInput.h"
#include <iterator>
#include "input/Input.h"
#ifdef USE_IMGUI
#include "manager/editor/DebugUIManager.h"
#include "externals/imgui/imgui.h"
#endif

namespace KCE
{
namespace
{
// 見るゲームパッドの番号。今は1人プレイ前提
constexpr DWORD kGamepadIndex = 0;

// デバッグ表示用の名前。enum の並びと合わせる
constexpr const char* kActionNames[] = { "MoveForward", "MoveBack", "MoveLeft", "MoveRight", "Jump", "Attack", "Pause" };
static_assert(std::size(kActionNames) == static_cast<size_t>(GameAction::Count), "アクション名の数が合わない");

constexpr const char* kLockNames[] = { "Intro", "Pause", "DebugCamera" };
static_assert(std::size(kLockNames) == static_cast<size_t>(InputLockReason::Count), "ロック名の数が合わない");

size_t ToIndex(GameAction action)
{
	return static_cast<size_t>(action);
}

uint32_t ToBit(InputLockReason reason)
{
	return 1u << static_cast<uint32_t>(reason);
}
} // namespace

std::unique_ptr<GameInput> GameInput::instance_ = nullptr;

GameInput* GameInput::GetInstance()
{
	if (!instance_)
	{
		instance_ = std::make_unique<GameInput>();
	}
	return instance_.get();
}

void GameInput::Initialize()
{
	for (auto& bindings : bindings_)
	{
		bindings.clear();
	}
	ignoreLock_.fill(false);
	current_.fill(false);
	previous_.fill(false);
	wasBlocked_.fill(false);
	lockMask_ = 0;
	debugTriggerCounts_.fill(0);
	debugReleaseCounts_.fill(0);

	SetDefaultBindings();

#ifdef USE_IMGUI
	// 呼び直されても二重に出さない
	DebugUIManager::GetInstance()->Unregister(this);
	DebugUIManager::GetInstance()->RegisterSettingsPage(this, "デバッグ", "ゲーム入力", [this]() { this->DrawImGui(); });
#endif
}

void GameInput::Finalize()
{
#ifdef USE_IMGUI
	if (DebugUIManager::HasInstance())
	{
		DebugUIManager::GetInstance()->Unregister(this);
	}
#endif
}

GameInput::~GameInput()
{
	Finalize();
}

void GameInput::SetDefaultBindings()
{
	using Device = InputBinding::Device;
	constexpr uint32_t kMouseLeft = 0;

	AddBinding(GameAction::MoveForward, { Device::Keyboard, DIK_W });
	AddBinding(GameAction::MoveForward, { Device::Keyboard, DIK_UP });
	AddBinding(GameAction::MoveForward, { Device::Gamepad, XINPUT_GAMEPAD_DPAD_UP });

	AddBinding(GameAction::MoveBack, { Device::Keyboard, DIK_S });
	AddBinding(GameAction::MoveBack, { Device::Keyboard, DIK_DOWN });
	AddBinding(GameAction::MoveBack, { Device::Gamepad, XINPUT_GAMEPAD_DPAD_DOWN });

	AddBinding(GameAction::MoveLeft, { Device::Keyboard, DIK_A });
	AddBinding(GameAction::MoveLeft, { Device::Keyboard, DIK_LEFT });
	AddBinding(GameAction::MoveLeft, { Device::Gamepad, XINPUT_GAMEPAD_DPAD_LEFT });

	AddBinding(GameAction::MoveRight, { Device::Keyboard, DIK_D });
	AddBinding(GameAction::MoveRight, { Device::Keyboard, DIK_RIGHT });
	AddBinding(GameAction::MoveRight, { Device::Gamepad, XINPUT_GAMEPAD_DPAD_RIGHT });

	AddBinding(GameAction::Jump, { Device::Keyboard, DIK_SPACE });
	AddBinding(GameAction::Jump, { Device::Gamepad, XINPUT_GAMEPAD_A });

	AddBinding(GameAction::Attack, { Device::Keyboard, DIK_J });
	AddBinding(GameAction::Attack, { Device::Mouse, kMouseLeft });
	AddBinding(GameAction::Attack, { Device::Gamepad, XINPUT_GAMEPAD_X });

	// ポーズはロック中でも開け閉めできるようにしておく
	AddBinding(GameAction::Pause, { Device::Keyboard, DIK_ESCAPE });
	AddBinding(GameAction::Pause, { Device::Gamepad, XINPUT_GAMEPAD_START });
	SetIgnoreLock(GameAction::Pause, true);
}

void GameInput::Update()
{
	previous_ = current_;

	for (size_t i = 0; i < kActionCount; ++i)
	{
		const GameAction action = static_cast<GameAction>(i);
		const bool rawPressed = ReadRawPressed(action);
		const bool blocked = IsBlocked(action);

		current_[i] = rawPressed && !blocked;

		// ロック解除の瞬間に押しっぱなしだった分は「押した瞬間」にしない
		if (wasBlocked_[i] && !blocked)
		{
			previous_[i] = current_[i];
		}
		wasBlocked_[i] = blocked;

#ifdef USE_IMGUI
		if (current_[i] && !previous_[i])
		{
			++debugTriggerCounts_[i];
		}
		if (!current_[i] && previous_[i])
		{
			++debugReleaseCounts_[i];
		}
#endif
	}
}

void GameInput::DrawImGui()
{
#ifdef USE_IMGUI
	ImGui::Text("Locked: %s", IsLocked() ? "yes" : "no");
	for (size_t i = 0; i < std::size(kLockNames); ++i)
	{
		const InputLockReason reason = static_cast<InputLockReason>(i);
		bool locked = IsLockedBy(reason);
		if (ImGui::Checkbox(kLockNames[i], &locked))
		{
			if (locked)
			{
				Lock(reason);
			}
			else
			{
				Unlock(reason);
			}
		}
	}

	ImGui::Separator();
	if (ImGui::BeginTable("GameInputActions", 4, ImGuiTableFlags_Borders))
	{
		ImGui::TableSetupColumn("Action");
		ImGui::TableSetupColumn("Pressed");
		ImGui::TableSetupColumn("Triggered");
		ImGui::TableSetupColumn("Released");
		ImGui::TableHeadersRow();
		for (size_t i = 0; i < kActionCount; ++i)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(kActionNames[i]);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(current_[i] ? "ON" : "-");
			ImGui::TableNextColumn();
			ImGui::Text("%d", debugTriggerCounts_[i]);
			ImGui::TableNextColumn();
			ImGui::Text("%d", debugReleaseCounts_[i]);
		}
		ImGui::EndTable();
	}

	if (ImGui::Button("Reset Counts"))
	{
		debugTriggerCounts_.fill(0);
		debugReleaseCounts_.fill(0);
	}
#endif
}

void GameInput::AddBinding(GameAction action, const InputBinding& binding)
{
	bindings_[ToIndex(action)].push_back(binding);
}

void GameInput::ClearBindings(GameAction action)
{
	bindings_[ToIndex(action)].clear();
}

void GameInput::SetIgnoreLock(GameAction action, bool ignore)
{
	ignoreLock_[ToIndex(action)] = ignore;
}

bool GameInput::IsPressed(GameAction action) const
{
	return current_[ToIndex(action)];
}

bool GameInput::IsTriggered(GameAction action) const
{
	const size_t i = ToIndex(action);
	return current_[i] && !previous_[i];
}

bool GameInput::IsReleased(GameAction action) const
{
	const size_t i = ToIndex(action);
	return !current_[i] && previous_[i];
}

void GameInput::Lock(InputLockReason reason)
{
	lockMask_ |= ToBit(reason);
}

void GameInput::Unlock(InputLockReason reason)
{
	lockMask_ &= ~ToBit(reason);
}

bool GameInput::IsLockedBy(InputLockReason reason) const
{
	return (lockMask_ & ToBit(reason)) != 0;
}

bool GameInput::ReadRawPressed(GameAction action) const
{
	const Input* input = Input::GetInstance();
	for (const InputBinding& binding : bindings_[ToIndex(action)])
	{
		bool pressed = false;
		switch (binding.device)
		{
		case InputBinding::Device::Keyboard:
			pressed = input->PushKey(static_cast<BYTE>(binding.code));
			break;
		case InputBinding::Device::Gamepad:
			pressed = input->IsButtonPressed(kGamepadIndex, binding.code);
			break;
		case InputBinding::Device::Mouse:
			pressed = input->IsMouseButtonPressed(static_cast<int>(binding.code));
			break;
		}
		if (pressed)
		{
			return true;
		}
	}
	return false;
}

bool GameInput::IsBlocked(GameAction action) const
{
	return IsLocked() && !ignoreLock_[ToIndex(action)];
}
} // namespace KCE
