#include <windows.h>
#include <stdio.h>
#include "svnrev.h"

int get_svn_revision(char *svnversion, char *path) {
	FILE *fp;
	char command[MAX_PATH*2];
	char result[32]= "";
	int revision = -1;
	char arg1[MAX_PATH], arg2[MAX_PATH];

	// subversion 1.7 から .svn\entries のフォーマットが変わったため、
	// .svn\entries を直接読み込むのをやめ、
	// svnversion.exe コマンドを呼び出した結果を返す

	// _popen はスペースが含まれる場合にダブルクォートで囲んでも
	// うまく動かないため 8.3 形式に変換
	GetShortPathName(svnversion, arg1, sizeof(arg1));
	GetShortPathName(path, arg2, sizeof(arg2));

	_snprintf_s(command, sizeof(command), _TRUNCATE, "%s -n %s", arg1, arg2);
	if ((fp = _popen(command, "rt")) == NULL ) {
		return -1;
	}

	fread(result, sizeof(result), sizeof(result)-1, fp);
	revision = atoi(result);
	_pclose(fp);

	if (revision == 0) {
		revision = -1;
	}

	return revision;
}

BOOL write_svn_revesion(char *filename, int revision) {
	FILE *fp;
	char buf[64] = "";
	int file_revision = -1;

	// print to stdout
	if (strcmp(filename, "-") == 0) {
		fprintf(stdout, "#define SVNVERSION %d\n", revision);
		return TRUE;
	}

	// read current file
	if (fopen_s(&fp, filename, "r") == 0) {
		fread(buf, sizeof(char), sizeof(buf)-1, fp);
		fclose(fp);

		sscanf_s(buf, "#define SVNVERSION %d", &file_revision);
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

	if (fopen_s(&fp, filename, "w+") != 0) {
		return FALSE;
	}

	if (revision >= 1) {
		fprintf(fp, "#define SVNVERSION %d\n", revision);
	}
	else {
		fprintf(fp, "#undef SVNVERSION\n");
	}

	fclose(fp);

	return TRUE;
}

int main(int argc, char* argv[])
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
