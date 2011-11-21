#include "stdafx.h"
#include "svnrev.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

int get_svn_revision(char *svnversion, char *path) {
	FILE *fp;
	char command[MAX_PATH*2];
	char result[32];
	int revision = -1;

	// subversion 1.7 から .svn\entries のフォーマットが変わったため、
	// .svn\entries を直接読み込むのをやめ、
	// svnversion.exe コマンドを呼び出した結果を返す
	_snprintf_s(command, sizeof(command), _TRUNCATE, "%s -n %s", svnversion, path);
	if ((fp = _popen(command, "rt")) == NULL ) {
		return -1;
	}

	while(!feof(fp)){
		fgets(result, sizeof(result), fp);
		revision = atoi(result);
		break;
	}
	_pclose(fp);

	return revision;
}

BOOL write_svn_revesion(char *filename, int revision) {
	BOOL ret;
	CStdioFile csf;
	CString cs;
	int file_revision = -1;
	
	// print to stdout
	if (strcmp(filename, "-") == 0) {
		CStdioFile csf (stdout);
		cs.Format("#define SVNVERSION %d\n", revision);
		csf.WriteString(cs);
		return TRUE;
	}

	// read current file
	ret = csf.Open(filename, CFile::modeRead);
	if (ret == TRUE) {
		csf.SeekToBegin();
		csf.ReadString(cs);
		csf.Close();

		ret = sscanf_s(cs, "#define SVNVERSION %d", &file_revision);
	}

	// compare revisions
	// .svn 配下のファイルが存在しない(-1)場合においても、ビルドができるように、
	// ヘッダファイルは作成する。企業のイントラネットから SourceForge のSVNリポジトリへ
	// 直接アクセスできない場合、tarballをダウンロードするしかなく、その際 .svn ディレクトリが
	// 存在しない。
	if (file_revision != -1 &&
		(file_revision >= revision)) {
		return TRUE;
	}

	ret = csf.Open(filename, CFile::modeWrite | CFile::modeCreate);
	if (ret == FALSE) {
		return FALSE;
	}

	if (revision >= 1) {
		cs.Format("#define SVNVERSION %d\n", revision);
		csf.WriteString(cs);
	}
	else {
		cs.Format("#undef SVNVERSION\n");
		csf.WriteString(cs);
	}

	csf.Close();

	return TRUE;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int revision = -1;
	char *svnversion, *input, *output;

	if (argc != 4) {
		printf("USAGE: %s svnversion path output\n", argv[0]);
		return -1;
	}

	svnversion = argv[1]; // svnversion.exe
	input = argv[2];      // top of source tree
	output = argv[3];     // output to
	revision = get_svn_revision(svnversion, input);

	if (!write_svn_revesion(output, revision)) {
		return 1;
	}

	return 0;
}
