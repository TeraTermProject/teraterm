/*
 * Copyright (C) 2009-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

// VS2005でビルドされたバイナリが Windows95 でも起動できるようにするために、
// IsDebuggerPresent()のシンボル定義を追加する。
//
// cf. https://pf-j.sakura.ne.jp/program/tips/forceimp.htm


#if _MSC_VER == 1400

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
    DWORD_PTR *lpdw;
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
    lpdw = (DWORD_PTR *) &_imp__IsDebuggerPresent;
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

#else /* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif
void __stdcall DoCover_IsDebuggerPresent()
{
	// NOP
}
#ifdef __cplusplus
}
#endif

#endif /* _MSC_VER */
