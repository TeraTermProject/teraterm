// VS2005でビルドされたバイナリが Windows95 でも起動できるようにするために、
// IsDebuggerPresent()のシンボル定義を追加する。
//
// cf.http://jet2.u-abel.net/program/tips/forceimp.htm

// 装飾された名前のアドレスを作るための仮定義
// (これだけでインポートを横取りしている)
#ifdef __cplusplus
extern "C" {
#endif
int WINAPI _imp__IsDebuggerPresent()
    { return PtrToInt((void*) &_imp__IsDebuggerPresent); }
#ifdef __cplusplus
}
#endif

// 実際に横取り処理を行う関数
#ifdef __cplusplus
extern "C" {
#endif
BOOL WINAPI Cover_IsDebuggerPresent()
    { return FALSE; }
#ifdef __cplusplus
}
#endif

// 関数が実際に呼び出されたときに備えて
// 横取り処理関数を呼び出させるための下準備
#ifdef __cplusplus
extern "C" {
#endif
void __stdcall DoCover_IsDebuggerPresent()
{
    DWORD dw;
    DWORD_PTR FAR* lpdw;
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    // Windows95 でなければここでおわり
    GetVersionEx(&osvi);
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT ||
        // VER_PLATFORM_WIN32_WINDOWS なら、あとは Minor だけで判定できる
        // osvi.dwMajorVersion > 4 ||
        osvi.dwMinorVersion > 0) {
        return;
    }
    // 横取り関数を設定するアドレスを取得
    lpdw = (DWORD_PTR FAR*) &_imp__IsDebuggerPresent;
    // このアドレスを書き込めるように設定
    // (同じプログラム内なので障害なく行える)
    VirtualProtect(lpdw, sizeof(DWORD_PTR), PAGE_READWRITE, &dw);
    // 横取り関数を設定
    *lpdw = (DWORD_PTR)(FARPROC) Cover_IsDebuggerPresent;
    // 読み書きの状態を元に戻す
    VirtualProtect(lpdw, sizeof(DWORD_PTR), dw, NULL);
}
#ifdef __cplusplus
}
#endif

// アプリケーションが初期化される前に下準備を呼び出す
// ※ かなり早くに初期化したいときは、このコードを
//  ファイルの末尾に書いて「#pragma init_seg(lib)」を、
//  この変数宣言の手前に書きます。
//  初期化を急ぐ必要が無い場合は WinMain 内から
//  DoCover_IsDebuggerPresent を呼び出して構いません。
/* C言語では以下のコードは、コンパイルエラーとなるので、WinMain, DllMain から呼ぶ。*/
#ifdef __cplusplus
extern "C" {
int s_DoCover_IsDebuggerPresent
    = (int) (DoCover_IsDebuggerPresent(), 0);
}
#endif
