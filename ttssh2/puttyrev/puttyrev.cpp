// puttyrev.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "puttyrev.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一のアプリケーション オブジェクトです。

CWinApp theApp;

using namespace std;

void write_putty_version(char *path)
{
	BOOL ret;
	CStdioFile csf;
	char *keywords[] = {
		//"AppVerName",
		"AppVersion",
		"VersionInfoTextVersion",
		NULL,
	};
	int i;
	CString filename, buf, cs;
	char revision[64] = {0};

	// PuTTYのバージョンを取得する。
	filename = path;
	filename += "\\libs\\putty\\windows\\putty.iss";

	ret = csf.Open(filename, CFile::modeRead);
	if (ret == FALSE) {
		goto write;
	}

	while (csf.ReadString(cs) != NULL) {
		CString tmp;
		for (i = 0 ; keywords[i] ; i++) {
			tmp = keywords[i];
			tmp += "=%[^\n]s";
			ret = sscanf_s(cs, tmp, revision, sizeof(revision));
			if (ret != 1) 
				continue;
			printf("%s\n", revision);
			goto close;
		}
	}

close:
	csf.Close();

write:
	// バージョンをヘッダに書き込む。
	filename = path;
	filename += "\\ttssh2\\ttxssh\\puttyversion.h";

	ret = csf.Open(filename, CFile::modeWrite | CFile::modeCreate);
	if (ret == FALSE) {
		goto end;
	}

	if (revision[0] != '\0') {
		cs.Format("#define PUTTYVERSION \"%s\"\n", revision);
		csf.WriteString(cs);
	}
	else {
		cs.Format("#undef PUTTYVERSION\n");
		csf.WriteString(cs);
	}

	csf.Close();

end:;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;
	char path[MAX_PATH * 2];
	int i, len;

	// MFC を初期化して、エラーの場合は結果を印刷します。
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: 必要に応じてエラー コードを変更してください。
		_tprintf(_T("致命的なエラー: MFC の初期化ができませんでした。\n"));
		nRetCode = 1;
	}
	else
	{
		// TODO: アプリケーションの動作を記述するコードをここに挿入してください。
		GetModuleFileName(::GetModuleHandle(NULL), path, sizeof(path));
		len = (int)strlen(path);
		for (i=len; i>=0; i--) {
			if (path[i] == '\\') {
				break;
			}
			path[i] = '\0';
		}
		SetCurrentDirectory(path); // teraterm\debug or teraterm\release
		SetCurrentDirectory("..\\..\\..\\"); // top of source tree
		GetCurrentDirectory(sizeof(path), path);

		write_putty_version(path);
	}

	return nRetCode;
}
