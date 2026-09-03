#include "MyGame.h"
#ifdef _DEBUG
#include <crtdbg.h>
#endif

//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#ifdef _DEBUG
	// Make CRT assertion text capturable by automated Debug runs instead of
	// exposing only the generic process exit code 3.
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
	//フレームワーク
	std::unique_ptr<KCE::Framework> game = std::make_unique<KCE::MyGame>();

	//実行
	game->Run();

	//終了
	return 0;
}
