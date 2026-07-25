#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "teraterm.h"

#include "ttlib_types.h"
#include "codeconv.h"
#include "asprintf.h"

#include "autofilename.h"

/**
 *	日付および時刻の変換文字列の安全な文字列への置き換え
 *	strftimeの未サポートフォーマットは無効な文字に置き換える
 *
 *	@param	fmt	置き換え対象の文字列
 */
void ConvertSafeStrFtimeFormat(wchar_t *fmt) {
	static const wchar_t *kws=L"aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYz%N";
	if (fmt != NULL) {
		while (*fmt != L'\0') {
			if (*fmt == L'%') {
				fmt++;
				if (*fmt == L'\0') {
					fmt--;
					*fmt=L'\0';
					break;
				}
				if (wcschr(kws, *fmt) == NULL) {
					*fmt = L'%';
				}
			}
			fmt++;
		}
	}
}

/**
 *	ファイル名文字列の置き換え
 *	次の文字を置き換える
 *		&h	ホスト名に置換
 *		&p	TCPポート番号に置換
 *		&u	ログオン中のユーザ名
 *
 *	@param	pcv
 *	@param	name	置き換える前の文字列(ファイル名)
 *	@return	置き換えられた文字列
 *			不要になったらfree()すること
 */
wchar_t *GetConvertFilenameW(const TComVar *pcv, const wchar_t *name)
{
  const TTTSet *pts = pcv->ts;
  wchar_t *host = NULL;
  wchar_t *port = NULL;
  wchar_t *user = NULL;

  switch(pcv->PortType) {
  case IdTCPIP:
    host = ToWcharA(pts->HostName);
    aswprintf(&port, L"%d", pts->TCPPort);
    break;
  case IdSerial:
    aswprintf(&host, L"COM%d", pts->ComPort);
    port = _wcsdup(L"");
    break;
  default:
    host = _wcsdup(L"");
    port = _wcsdup(L"");
    break;
  }
  wchar_t buf[256 + 1];	// 256=UNLEN
  DWORD l = _countof(buf);
  if (GetUserNameW(buf, &l) != 0) {
    user = _wcsdup(buf);
  }
  else {
    user = _wcsdup(L"");
  }

	size_t len = wcslen(name) + 1;
	const wchar_t *s = name;

  while(*s != '\0') {
		if (*s == '&') {
      s ++;
      if (*s == '\0') {
        break;
      }
      switch (*s) {
      case 'h':
        len += wcslen(host);
        break;
      case 'p':
        len += wcslen(port);
        break;
      case 'u':
        len += wcslen(user);
        break;
      default:
        ;
        break;
      }
    }
    s ++;
  }

  wchar_t *res = (wchar_t *)malloc(sizeof(wchar_t) * len);
  size_t i = 0;
	s = name;
	while(*s != '\0') {
		if (*s != '&') {
      res[i++] = *s++;
      continue;
    }
    s ++;
    if (*s == '\0') {
      break;
    }
    switch (*s) {
    case 'h':
      if (host[0] == L'\0') break;
      wcscpy(&res[i], host);
      i += wcslen(host);
      break;
    case 'p':
      if (port[0] == L'\0') break;
      wcscpy(&res[i], port);
      i += wcslen(port);
      break;
		case 'u':
      if (user[0] == L'\0') break;
      wcscpy(&res[i], user);
      i += wcslen(user);
      break;
		default:
      break;
		}
    s ++;
	}
	res[i] = L'\0';

  free(user);
  free(port);
  free(host);
	return res;
}

/**
 * 自動保存ファイル名取得
 * 
 * ディレクトリとファイル名の組み合わせ
 * filename なしの場合：         (path_def, name_def)
 * filename は相対パスでない場合：(filename ディレクトリ部, filename ファイル名部)
 * filename は相対パスの場合：    (path_def, filename)
 * 
 * ディレクトリなしなら GetTermLogDir(pcv->ts)
 *
 *	@param	pcv
 *	@param	filename	置き換える前の文字列(ファイル名)
 *	@param	path_def	保存先ディレクトリパスの初期値
 *	@param	name_def	ファイル名の初期値
 *	@return	置き換えられた文字列
 *			不要になったらfree()すること
 */
wchar_t *GetAutoFilename(const TComVar *pcv, const wchar_t *filename, const wchar_t *path_def, const wchar_t *name_def)
{
  const TTTSet *pts = pcv->ts;
  wchar_t *dir;
  wchar_t *fname;
  if (filename == NULL || filename[0] == L'\0') {
    dir = _wcsdup(path_def);
    fname = _wcsdup(name_def);
  }
  else if (!IsRelativePathW(filename)) {
    dir = ExtractDirNameW(filename);
    fname = ExtractFileNameW(filename);
  }
  else {
    dir = _wcsdup(path_def);
    fname = _wcsdup(filename);
  }

  if (fname != NULL && fname[0] != L'\0') {
    ConvertSafeStrFtimeFormat(fname);

    time_t time_local;
    time(&time_local);
    struct tm tm_local;
    localtime_s(&tm_local, &time_local);

    size_t len = wcslen(fname) + 32;
    wchar_t *buf = (wchar_t *)malloc(len);
    size_t r = wcsftime(buf, len, fname, &tm_local);
    if (r != 0) {
      free(fname);
      fname = buf;
    }
  }

  if (fname != NULL && fname[0] != L'\0') {
  	wchar_t *buf = GetConvertFilenameW(pcv, fname);
    free(fname);
    fname = buf;
  }

  if (dir == NULL || dir[0] == L'\0') {
    free(dir);
  	dir = GetTermLogDir(pts);
  }
  if (fname == NULL || fname[0] == L'\0') {
    free(fname);
  	fname = _wcsdup(name_def);
  }
  else {
	  wchar_t *replaced = replaceInvalidFileNameCharW(fname, L'_');
    free(fname);
  	fname = replaced;
  }

	wchar_t *full = GetFullPathW(dir, fname);
	free(fname);
	free(dir);

	return full;
}
