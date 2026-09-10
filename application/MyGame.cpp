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

	Audio::GetInstance()->Load("Cozy_rain.mp3", SoundGroup::BGM);
	Audio::GetInstance()->PlayWave("Cozy_rain.mp3", true);

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
	};

	// テクスチャの読み込み
	LoadTextures();

	// モデルの読み込み
	LoadModels();

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
	// エディタではシーンをImGuiのウィンドウに表示するため、
	// バックバッファではなくレンダーターゲットへ出力する。
	Framework::ExecuteRenderPipeline(sceneRenderTexture_.get());

	// バックバッファのクリア
	dxCommon_->PreDraw();

	// メインメニューバー
	DebugUIManager* debugUIManager = DebugUIManager::GetInstance();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Window"))
		{
			bool showHierarchy = debugUIManager->IsShowHierarchy();
			if (ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy)) debugUIManager->SetShowHierarchy(showHierarchy);

			bool showInspector = debugUIManager->IsShowInspector();
			if (ImGui::MenuItem("Inspector", nullptr, &showInspector)) debugUIManager->SetShowInspector(showInspector);

			bool showConsole = debugUIManager->IsShowConsole();
			if (ImGui::MenuItem("Console", nullptr, &showConsole)) debugUIManager->SetShowConsole(showConsole);

			bool showProject = debugUIManager->IsShowProject();
			if (ImGui::MenuItem("Project", nullptr, &showProject)) debugUIManager->SetShowProject(showProject);

			ImGui::Separator();
			if (ImGui::BeginMenu("UI Scale"))
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
			if (ImGui::MenuItem("Reset Layout"))
			{
				debugUIManager->RequestLayoutReset();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools"))
		{
			debugUIManager->DrawToolsMenu();
			ImGui::EndMenu();
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
		debugUIManager->ClearLayoutResetRequest();

		ImGui::DockBuilderRemoveNode(dockspace_id); // 既存レイアウト削除
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // 空ノード追加
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

		ImGuiID dock_main_id = dockspace_id;
		ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
		ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
		ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

		// ウィンドウのドッキング
		ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left);
		ImGui::DockBuilderDockWindow("Performance", dock_id_left);

		ImGui::DockBuilderDockWindow("Scene", dock_main_id);

		ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
		ImGui::DockBuilderDockWindow("SceneManager", dock_id_right);
		ImGui::DockBuilderDockWindow("Time Manager", dock_id_right);
		ImGui::DockBuilderDockWindow("TimerManager", dock_id_right);

		ImGui::DockBuilderDockWindow("Sequencer Inspector", dock_id_right);

		ImGui::DockBuilderDockWindow("Sequencer", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Project", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Console", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Audio Debug", dock_id_bottom);
		ImGui::DockBuilderDockWindow("JSON Editor", dock_id_bottom);

		ImGui::DockBuilderFinish(dockspace_id);
	}

	// シーンウィンドウ
	ImGui::Begin("Scene");
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

	// 登録されたSceneエリアのデバッグUIを描画する
	debugUIManager->DrawArea(DebugUIArea::Scene);

	ImGui::End();

	// 各種標準デバッグウィンドウの描画
	if (debugUIManager->IsShowHierarchy())
	{
		bool open = debugUIManager->IsShowHierarchy();
		if (ImGui::Begin("Hierarchy", &open))
		{
			debugUIManager->DrawArea(DebugUIArea::Hierarchy);
		}
		ImGui::End();
		debugUIManager->SetShowHierarchy(open);
	}

	if (debugUIManager->IsShowInspector())
	{
		bool open = debugUIManager->IsShowInspector();
		if (ImGui::Begin("Inspector", &open))
		{
			debugUIManager->DrawArea(DebugUIArea::Inspector);
		}
		ImGui::End();
		debugUIManager->SetShowInspector(open);
	}

	if (debugUIManager->IsShowProject())
	{
		bool open = debugUIManager->IsShowProject();
		if (ImGui::Begin("Project", &open))
		{
			debugUIManager->DrawArea(DebugUIArea::Project);
		}
		ImGui::End();
		debugUIManager->SetShowProject(open);
	}

	if (debugUIManager->IsShowConsole())
	{
		bool open = debugUIManager->IsShowConsole();
		ConsoleLog::GetInstance()->Draw(&open);
		debugUIManager->SetShowConsole(open);
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
} // namespace KCE
