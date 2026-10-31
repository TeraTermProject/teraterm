/*
 * Copyright (C) 2026- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//
// Tera Term launcher
//	ttlaunch.cpp と ttlaunch_setup.cpp で共有する定義
//

#pragma once

#include <windows.h>
#include <string>
#include <vector>

typedef enum {
	SYSTEM_CYGWIN,
	SYSTEM_MSYS2,
	SYSTEM_GIT_BASH
} system_t;

/**
 *	ポップアップに表示する起動方法
 */
struct LaunchItem {
	std::wstring id;		// -e で指定する名前
	std::wstring label;		// メニューに表示する文字列
	int group;				// グループが変わるところにセパレータを入れる
	system_t system;		// cygterm / msys2term のどちらを使うか
	std::wstring root;		// cygwin / msys2 / git のインストールフォルダ
	std::wstring args;		// cygterm(msys2term) に渡す引数
	bool posix_shell;		// cygwin/msys2 のシェル(CHERE_INVOKINGを設定する)
	std::wstring icon;		// メニューに表示するアイコン(.ico または .exe), 無いときは ""
	std::wstring alias;		// -e で指定できる別名, 無いときは ""
};

/**
 *	文言ファイル (ttlaunch.lng)
 *		ttlaunch.exe と同じフォルダの ttlaunch.lng を使用する
 *		セクション名は言語名で、Tera Term の設定 (TERATERM.INI の UILanguageFile) の
 *		lng ファイルの [Info] language= ("Japanese(日本語)" など) の括弧より前の部分
 *		セクションやキーが無いときはプログラム内の文言 (英語) になる
 */
struct LangInfo {
	std::wstring file;		// ttlaunch.lng のフルパス
	std::string section;	// 言語名 (セクション名)
};

/**
 *	コマンドライン引数として渡せるようクォートする
 */
std::wstring QuoteArg(const std::wstring &arg);

/**
 *	ttlaunch.exe のあるフォルダ
 */
std::wstring GetExeDir(void);

/**
 *	文言ファイルの情報 (初回に読み込んで保持する)
 */
const LangInfo &GetLangInfo(void);

/**
 *	文言ファイルから文言を取得する
 *	@param	lang	文言ファイルの情報
 *	@param	key		キー
 *	@param	def		無いときの文言
 */
std::wstring GetLangStr(const LangInfo &lang, const char *key, const wchar_t *def);

/**
 *	文言に %s を埋め込む
 */
std::wstring FormatLangStr(const LangInfo &lang, const char *key, const wchar_t *def, const std::wstring &arg);
