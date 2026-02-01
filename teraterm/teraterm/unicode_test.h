/*
 * テストのために追加
 *	teraterm.hなどに入れると依存ファイルが多いためビルドに時間がかかるため
 *	テスト用ヘッダに分離
 *	将来はなくなる
 *
 */
#pragma once

// デバグ用機能enable
//		CTRL x2でマウスポインタ下の文字表法を表示
#define	UNICODE_DEBUG			1

// カーソル点滅系を止めるデバグ用
//		描画が最低限となる
//		1でカーソルの点滅処理が行われなくなる
#define	UNICODE_DEBUG_CARET_OFF	0
