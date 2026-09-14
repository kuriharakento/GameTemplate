#include "MyGame.h"

#include <future>
#include <chrono>
#include <filesystem>
#include <windows.h>
#include <Psapi.h>
#include "manager/graphics/ModelManager.h"
#include "manager/graphics/TextureManager.h"
#include "base/Logger.h"
#include "effects/particle/ParticleManager.h"
#include "externals/imgui/imgui_internal.h"
#include "manager/editor/DebugUIManager.h"
#include "manager/editor/ConsoleLog.h"
#include "manager/graphics/LineManager.h"
#include "input/Input.h"
#include "audio/Audio.h"

#include "base/PathManager.h"
#include "editor/SceneViewContext.h"

namespace KCE
{
///=============================================================================
///						初期化・終了処理
///=============================================================================

void MyGame::Initialize()
{
	// アプリケーションリソースルートの設定
	PathManager::SetApplicationResourceRoot("application/Resources");

	// フレームワークの初期化
	Framework::Initialize();

	// BGM のデコードは重いので、テクスチャとモデルを読んでる間にワーカーで回す。
	// 鳴らすまでメインスレッドから Audio を触らないこと
	jobSystem_->Submit([]() { Audio::GetInstance()->Load("Cozy_rain.mp3", SoundGroup::BGM); });

	// ゲーム側でウィンドウタイトルを決める
	winApp_->SetWindowTitle(L"MyGame");

	// シーンコンテキストの作成
	SceneContext context;
	context = {
		spriteCommon_.get(),
		objectCommon_.get(),
		cameraManager_.get(),
		lightManager_.get(),
		postProcessManager_.get(),
		skybox_.get(),
		shadowMapManager_.get(),
		this,
		depthOfFieldRenderer_.get(),
		volumetricLightRenderer_.get(),
		planarReflection_.get(),
		text3DRenderer_.get(),
	};
	context.dxCommon = dxCommon_.get();
	context.srvManager = srvManager_.get();

	// テクスチャの読み込み
	LoadTextures();

	// モデルの読み込み
	LoadModels();

	// BGM のデコードが終わってから鳴らす
	jobSystem_->WaitIdle();
	Audio::GetInstance()->PlayWave("Cozy_rain.mp3", true);

	// GPUの完了待ちをしてから中間リソースを解放
	dxCommon_->ExecuteAndWait();
	TextureManager::GetInstance()->ClearIntermediateResources();

	// ゲームの初期化処理
	sceneManager_->Initialize(context);

	// Skyboxの初期化
	skybox_->Initialize(dxCommon_.get(), "./Resources/skybox.dds");

	// シーン描画用レンダーテクスチャ（ポストプロセス後）の初期化
	constexpr float kClearColorValue = 0.1f;  // 暗いグレー
	Vector4 clearColor = { kClearColorValue, kClearColorValue, kClearColorValue, 1.0f };
	sceneRenderTexture_ = std::make_unique<RenderTexture>();
	sceneRenderTexture_->Initialize(
		dxCommon_.get(),
		srvManager_.get(),
		winApp_->GetClientWidth(),
		winApp_->GetClientHeight(),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		clearColor
	);
}

void MyGame::Finalize()
{
	// ゲームの終了処理
	sceneManager_.reset();

	// フレームワークの終了処理
	Framework::Finalize();
}

///=============================================================================
///						更新処理
///=============================================================================

void MyGame::Update()
{
	// フレームワークの更新処理
	Framework::Update();

	// パフォーマンス情報の表示
	Framework::ShowPerformanceInfo();

	// ゲームの更新処理
	sceneManager_->Update();

	// パーティクルマネージャーの更新
	ParticleManager::GetInstance()->Update(cameraManager_.get());
}

void MyGame::OnResize(uint32_t width, uint32_t height)
{
	if (sceneRenderTexture_)
	{
		sceneRenderTexture_->Resize(width, height);
	}
}

///=============================================================================
///						描画処理
///=============================================================================

void MyGame::Draw()
{
	srvManager_->PreDraw();

	// 描画順は engine 側の RenderPipeline に集約されている。
	// パスを足すときは Framework::GetRenderPipeline() から差し込むこと。
#ifdef USE_IMGUI
	HandleGameViewToggle();
	if (gameViewOnly_)
	{
		// ゲーム画面だけを出す。Release と同じくバックバッファへ直接描き、
		// ImGui は組み立てだけ終えて描かない（どこかのウィンドウが残って出ることもない）
		Framework::ExecuteRenderPipeline(nullptr);

		// マウスとギズモの基準をウィンドウ全体に合わせる
		POINT clientOrigin = { 0, 0 };
		ClientToScreen(winApp_->GetHwnd(), &clientOrigin);
		const float clientWidth = static_cast<float>(winApp_->GetClientWidth());
		const float clientHeight = static_cast<float>(winApp_->GetClientHeight());
		Input::GetInstance()->SetMouseCorrection({ 0.0f, 0.0f }, { clientWidth, clientHeight });
		SceneViewRect fullRect;
		fullRect.x = static_cast<float>(clientOrigin.x);
		fullRect.y = static_cast<float>(clientOrigin.y);
		fullRect.width = clientWidth;
		fullRect.height = clientHeight;
		SceneViewContext::GetInstance()->SetViewportRect(fullRect);
		SceneViewContext::GetInstance()->SetCamera(cameraManager_->GetActiveCamera());
		// ImGui のウィンドウが無いので、マウスは常にゲームへ渡す
		SceneViewContext::GetInstance()->SetHovered(true);

		imguiManager_->End();
		dxCommon_->PostDraw();
		return;
	}

	// エディタではシーンをImGuiのウィンドウに表示するため、
	// バックバッファではなくレンダーターゲットへ出力する。
	Framework::ExecuteRenderPipeline(sceneRenderTexture_.get());

	// バックバッファのクリア
	dxCommon_->PreDraw();

	// メインメニューバー
	DebugUIManager* debugUIManager = DebugUIManager::GetInstance();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("表示"))
		{
			debugUIManager->DrawWindowMenu();
			ImGui::Separator();
			if (ImGui::BeginMenu("UIの大きさ"))
			{
				float currentScale = debugUIManager->GetUIScale();
				float scales[] = { 0.50f, 0.75f, 1.00f, 1.25f, 1.50f, 1.75f, 2.00f };
				for (float s : scales)
				{
					char label[32];
					sprintf_s(label, "%d%%", static_cast<int>(s * 100.0f));
					bool selected = (currentScale == s);
					if (ImGui::MenuItem(label, nullptr, &selected))
					{
						debugUIManager->SetUIScale(s);
					}
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::MenuItem("レイアウトをリセット"))
			{
				debugUIManager->RequestLayoutReset();
			}

			ImGui::EndMenu();
		}

		// ImGui を全部消して、ゲーム画面だけを確認する。F11 でも切り替わる
		if (ImGui::MenuItem("ゲーム画面 (F11)"))
		{
			gameViewOnly_ = true;
		}

		// メニューバー中央にエンジン名を表示
		float menuBarWidth = ImGui::GetWindowWidth();
		float textWidth = ImGui::CalcTextSize("KentoCompoEngine").x;
		ImGui::SameLine((menuBarWidth - textWidth) * 0.5f);
		ImGui::TextUnformatted("KentoCompoEngine");

		// 右端にPerformance（FPSとメモリ使用率）を表示
		float fps = ImGui::GetIO().Framerate;
		PROCESS_MEMORY_COUNTERS memInfo;
		GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo));
		float memUsage = memInfo.WorkingSetSize / (1024.0f * 1024.0f);

		char perfBuf[64];
		sprintf_s(perfBuf, "FPS: %.2f | Mem: %.2f MB", fps, memUsage);
		float perfTextWidth = ImGui::CalcTextSize(perfBuf).x;
		ImGui::SameLine(menuBarWidth - perfTextWidth - 20.0f);
		ImGui::TextUnformatted(perfBuf);

		ImGui::EndMainMenuBar();
	}

	// ImGui ドッキングスペースの作成
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, viewport, ImGuiDockNodeFlags_None);

	// 初回起動時またはレイアウトリセット要求時に初期ドッキングレイアウトを自動構築
	static bool firstFrame = true;
	// 初期レイアウトの後に最初に見せるタブの数。選択タブはフォーカスに合わせて切り替わるので、1フレームに1つずつ当てる
	constexpr int kTabFocusSteps = 2;
	static int pendingTabFocusSteps = 0;
	if (firstFrame)
	{
		firstFrame = false;
		// imgui.ini が存在しない場合のみ初期レイアウトを構築する
		if (!std::filesystem::exists("imgui.ini"))
		{
			debugUIManager->RequestLayoutReset();
		}
	}

	if (debugUIManager->IsLayoutResetRequested())
	{
		constexpr float kBottomAreaRatio = 0.35f;
		constexpr float kLeftAreaRatio = 0.20f;
		// 右の列はタブが多いので、狭いとタブ名が途中で切れて見分けられない。少し広めに取る
		constexpr float kRightAreaRatio = 0.35f;
		constexpr float kRightBottomRatio = 0.50f;
		debugUIManager->ClearLayoutResetRequest();

		ImGui::DockBuilderRemoveNode(dockspace_id); // 既存レイアウト削除
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // 空ノード追加
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

		ImGuiID dock_main_id = dockspace_id;
		ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, kBottomAreaRatio, nullptr, &dock_main_id);
		ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, kLeftAreaRatio, nullptr, &dock_main_id);
		ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, kRightAreaRatio, nullptr, &dock_main_id);
		ImGuiID dock_id_right_bottom = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Down, kRightBottomRatio, nullptr, &dock_id_right);

		const auto dockWindows = [debugUIManager](EditorDock location, ImGuiID dockId)
		{
			for (const auto& name : debugUIManager->GetDockWindowNames(location))
			{
				ImGui::DockBuilderDockWindow(name.c_str(), dockId);
			}
		};
		dockWindows(EditorDock::Left, dock_id_left);
		ImGui::DockBuilderDockWindow("Scene###Scene", dock_main_id);
		dockWindows(EditorDock::Right, dock_id_right);
		dockWindows(EditorDock::RightBottom, dock_id_right_bottom);
		dockWindows(EditorDock::Bottom, dock_id_bottom);
		// Console は ConsoleLog が自分で描いていて登録一覧に入らないので、ここで下の段に入れる
		ImGui::DockBuilderDockWindow("###Console", dock_id_bottom);
		ImGui::DockBuilderFinish(dockspace_id);
		pendingTabFocusSteps = kTabFocusSteps;
	}

	// シーンウィンドウ
	ImGui::Begin("Scene###Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	ImGui::Image((ImTextureID)sceneRenderTexture_->GetGPUHandle().ptr, viewportSize);

	// シーンウィンドウの描画領域に合わせてマウス入力を補正する
	ImVec2 imagePos = ImGui::GetItemRectMin();
	ImVec2 imageSize = ImGui::GetItemRectSize();

	POINT clientOrigin = { 0, 0 };
	ClientToScreen(winApp_->GetHwnd(), &clientOrigin);

	KCE::Vector2 offset = { imagePos.x - clientOrigin.x, imagePos.y - clientOrigin.y };
	KCE::Vector2 size = { imageSize.x, imageSize.y };
	Input::GetInstance()->SetMouseCorrection(offset, size);

	// ギズモを重ねて描けるよう、シーン画像の矩形と描画カメラをエンジン側へ渡す
	SceneViewRect sceneViewRect;
	sceneViewRect.x = imagePos.x;
	sceneViewRect.y = imagePos.y;
	sceneViewRect.width = imageSize.x;
	sceneViewRect.height = imageSize.y;
	SceneViewContext::GetInstance()->SetViewportRect(sceneViewRect);
	SceneViewContext::GetInstance()->SetCamera(cameraManager_->GetActiveCamera());
	// シーン画像も ImGui のウィンドウの中なので、上にあるだけで ImGui がマウスを使っている扱いになる。
	// 画像の上かどうかを渡しておき、Input がそこではマウスをゲームへ渡せるようにする（直前の項目はシーン画像）
	SceneViewContext::GetInstance()->SetHovered(ImGui::IsItemHovered());

	// 登録されたSceneエリアのデバッグUIを描画する
	debugUIManager->DrawSceneOverlays();

	ImGui::End();

	// 登録UIはそれぞれ独立ウィンドウとして描く。
	debugUIManager->Draw();

	if (debugUIManager->IsShowConsole())
	{
		bool open = debugUIManager->IsShowConsole();
		ConsoleLog::GetInstance()->Draw(&open);
		debugUIManager->SetShowConsole(open);
	}
	if (pendingTabFocusSteps > 0)
	{
		// 全ウィンドウを作った後なら、ドッキング先の選択タブも確実に切り替わる。
		// ただし同じフレームで2回フォーカスすると後の方しか効かないので、フレームを分ける
		if (pendingTabFocusSteps == kTabFocusSteps)
		{
			// 右上は選んだ物を編集する流れが多いので、Inspector を最初に見せる
			ImGui::SetWindowFocus("###Inspector");
		}
		else
		{
			ImGui::SetWindowFocus("###Sequencer");
		}
		--pendingTabFocusSteps;
	}



#else
	// バックバッファへ直接出力する。
	// クリアのタイミングもパイプライン内のパスが面倒を見る。
	Framework::ExecuteRenderPipeline(nullptr);
#endif

	imguiManager_->End();
	imguiManager_->Draw();
	dxCommon_->PostDraw();
}

#ifdef USE_IMGUI
void MyGame::HandleGameViewToggle()
{
	// 文字を打っている間は取らない。ほかのショートカットとそろえる
	if (ImGui::GetIO().WantTextInput)
	{
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_F11, false))
	{
		gameViewOnly_ = !gameViewOnly_;
	}
}
#endif
} // namespace KCE
