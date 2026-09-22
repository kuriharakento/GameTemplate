# GameTemplate

自作の DirectX 12 ゲームエンジン [KentoCompoEngine](https://github.com/kuriharakento/KentoCompoEngine) を使って、ゲームを作り始めるためのテンプレートです。
エンジンは `engine/` に Git サブモジュールとして入っています。

## ビルドステータス

GameTemplate

[![windows_build_test](https://github.com/kuriharakento/GameTemplate/actions/workflows/windows_build_test.yml/badge.svg)](https://github.com/kuriharakento/GameTemplate/actions/workflows/windows_build_test.yml)

KentoCompoEngine

[![DebugBuild](https://github.com/kuriharakento/KentoCompoEngine/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/kuriharakento/KentoCompoEngine/actions/workflows/DebugBuild.yml)
[![ReleaseBuild](https://github.com/kuriharakento/KentoCompoEngine/actions/workflows/ReleaseBuild.yml/badge.svg)](https://github.com/kuriharakento/KentoCompoEngine/actions/workflows/ReleaseBuild.yml)

## 必要な環境

| 項目 | 内容 |
|------|------|
| OS | Windows 10 / 11 |
| IDE | Visual Studio 2026（プラットフォームツールセット v145） |
| ワークロード | 「C++ によるデスクトップ開発」 |
| SDK | Windows 10 SDK（10.0 以降） |
| GPU | DirectX 12 に対応したもの |
| 言語 | C++20 |

## ビルド手順

1. サブモジュールごと clone します。

   ```bash
   git clone --recursive https://github.com/kuriharakento/GameTemplate.git
   ```

   `--recursive` を付けずに clone した場合は、リポジトリの中で次を実行してください。

   ```bash
   git submodule update --init
   ```

2. リポジトリ直下の `GameTemplate.sln` を Visual Studio で開きます。
3. 構成を `Debug`、プラットフォームを `x64` にします。
4. **[ビルド] → [ソリューションのビルド]** を実行します。
5. `F5`（デバッグ開始）で起動します。

- `Debug` はエディタ（ImGui）付き、`Release` はゲーム画面だけで起動します。
- 実行ファイルは `generated/outputs/<構成>/GameTemplate.exe` に出力されます。

## 操作方法

### エディタ（Debug）

| 操作 | 内容 |
|------|------|
| `F9` | デバッグカメラのオン・オフ（メニューバーの「デバッグカメラ」でも切り替え可） |
| デバッグカメラ中に Scene の上で右ドラッグ | 視点を回す |
| 右ドラッグ中に `W` `A` `S` `D` / `Space` / `LShift` | 前後左右 / 上昇 / 下降 |
| `F11` | エディタの表示と、ゲーム画面だけの表示を切り替え |
| `F12` | フルスクリーンの切り替え |

デバッグカメラがオンの間は、ゲームの操作（移動やジャンプ）は止まります。

### ゲームの初期キー配置

| アクション | キーボード / マウス | ゲームパッド |
|------------|---------------------|--------------|
| 移動 | `W` `A` `S` `D` / 矢印キー | 十字ボタン |
| ジャンプ | `Space` | A |
| 攻撃 | `J` / 左クリック | X |
| ポーズ | `Esc` | START |

キー配置は `application/input/GameInput.cpp` の `SetDefaultBindings` で変えられます。

## フォルダ構成

```
GameTemplate/
├─ application/        ゲーム側のコード
│  ├─ MyGame.cpp       ゲーム全体の設定と、毎フレームの流れ
│  ├─ input/           GameInput（アクションとキーの対応）
│  ├─ scene/           シーン
│  └─ Resources/       モデル・テクスチャ・音・json
├─ engine/             KentoCompoEngine（サブモジュール）
└─ GameTemplate.sln
```

## ゲームごとの設定

ウィンドウのタイトルや最初に開くシーンは、`application/MyGame.cpp` の `CreateGameConfig` で決めます。

```cpp
GameConfig MyGame::CreateGameConfig() const
{
	GameConfig config;
	config.windowTitle = L"MyGame";
	config.startSceneName = "TitleScene";
	return config;
}
```

実行ファイルの名前は、Visual Studio のプロジェクトのプロパティ「全般 > ターゲット名」で変えられます。

## よくあるつまずき

| 症状 | 対処 |
|------|------|
| `engine/` フォルダが空で、ビルドが通らない | `git submodule update --init` を実行する |
| ビルドで `LNK1168` が出る | ゲームが起動したままなので、閉じてからビルドし直す |
| 追加したファイルが Visual Studio のフォルダ表示に出ない | リポジトリ直下で `python UpdateFilters.py` を実行する |

## 関連リンク

- [KentoCompoEngine](https://github.com/kuriharakento/KentoCompoEngine)
- [KentoCompoEngine API Reference](https://kuriharakento.github.io/KentoCompoEngine/)

## 作者

**kuriharakento**
[@kuriharakento](https://github.com/kuriharakento)
