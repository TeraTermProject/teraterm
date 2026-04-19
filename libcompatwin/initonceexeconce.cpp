#include <windows.h>

extern "C" BOOL WINAPI _InitOnceExecuteOnce(
	INIT_ONCE *flag,
	PINIT_ONCE_FN initFunc,
    void *Parameter,
	void **Context)
{
	/*
	 *	flag
	 *		0:未初期化
	 *		1:初期化中
	 *		2:完了
	 */
	while (1) {
		LONG old = InterlockedCompareExchange((LONG *)flag, 1, 0);
		if (old == 2) {
			// 初期化済み
			return TRUE;
		}
		if (old == 0) {
			// 未初期化、自分が初期化を行う
			initFunc(flag, Parameter, Context);
			InterlockedExchange((LONG *)flag, 2);
			return TRUE;
		}
		// 別スレッドが初期化中
		Sleep(1);
	}
	return TRUE;
}
