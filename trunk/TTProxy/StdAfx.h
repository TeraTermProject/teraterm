// stdafx.h : 標準のシステム インクルード ファイル、
//            または参照回数が多く、かつあまり変更されない
//            プロジェクト専用のインクルード ファイルを記述します。
//

#if !defined(AFX_STDAFX_H__B24257E3_70C7_482E_8DB9_1A5C4AE8B2F6__INCLUDED_)
#define AFX_STDAFX_H__B24257E3_70C7_482E_8DB9_1A5C4AE8B2F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _WINSOCKAPI_

// この位置にヘッダーを挿入してください
#include <winsock2.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#include <teraterm.h>
#include <tttypes.h>
#include <ttplugin.h>

// TODO: プログラムで必要なヘッダー参照を追加してください。

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_STDAFX_H__B24257E3_70C7_482E_8DB9_1A5C4AE8B2F6__INCLUDED_)
