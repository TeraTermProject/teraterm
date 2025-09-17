// stdafx.h : 標準のシステム インクルード ファイル、
//            または参照回数が多く、かつあまり変更されない
//            プロジェクト専用のインクルード ファイルを記述します。
//

#if !defined(AFX_STDAFX_H__B24257E3_70C7_482E_8DB9_1A5C4AE8B2F6__INCLUDED_)
#define AFX_STDAFX_H__B24257E3_70C7_482E_8DB9_1A5C4AE8B2F6__INCLUDED_

#pragma once

#define _WINSOCKAPI_

/* VS2015(VC14.0)だと、WSASocketA(), inet_ntoa() などのAPIがdeprecatedであると
* 警告するために、警告を抑止する。代替関数に置換すると、VS2005(VC8.0)でビルド
* できなくなるため、警告を抑止するだけとする。
*/
#if _MSC_VER >= 1800  // VSC2013(VC12.0) or later
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#endif

// この位置にヘッダーを挿入してください
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>

#include <teraterm.h>
#include <tttypes.h>
#include <ttplugin.h>

// TODO: プログラムで必要なヘッダー参照を追加してください。

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_STDAFX_H__B24257E3_70C7_482E_8DB9_1A5C4AE8B2F6__INCLUDED_)
