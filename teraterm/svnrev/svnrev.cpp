#include "stdafx.h"
#include "svnrev.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

// Get revision fron "entries" file.
int get_svn_revision(char *path) {
	BOOL ret;
	CStdioFile csf;
	CString cs, filename;
	int format = -1;
	int revision = -1;

	filename = path;
	if (filename.Right(1) != "\\") {
		filename += "\\";
	}
	filename += ".svn\\entries"; // [top of source tree]\.svn\entries

	ret = csf.Open(filename, CFile::modeRead);
	if (ret == FALSE) {
		return -1;
	}

	csf.SeekToBegin();

	// line 1
	csf.ReadString(cs);
	format = atoi(cs);

	if (format == 8 || format == 9 || format == 10) {
		// skip line 2 name, 3 kind
		csf.ReadString(cs);
		csf.ReadString(cs);

		// line 4 revision
		csf.ReadString(cs);
		revision = atoi(cs);
	}

	csf.Close();

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
	char *input, *output;

	if (argc != 3) {
		printf("USAGE: %s path output\n", argv[0]);
		return -1;
	}

	input = argv[1];
	output = argv[2];
	revision = get_svn_revision(input);

	if (!write_svn_revesion(output, revision)) {
		return 1;
	}

	return 0;
}
