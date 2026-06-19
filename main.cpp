#include "engine/scene/MyGame.h"

//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	//フレームワーク
	std::unique_ptr<Framework> game = std::make_unique<MyGame>();

	//実行
	game->Run();

	//終了
	return 0;
}