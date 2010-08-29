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
	filename += "\\.svn\\entries"; // [top of source tree]\.svn\entries

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

BOOL write_svn_revesion(char *path, int revision) {
	BOOL ret;
	CStdioFile csf;
	CString cs, filename;
	int file_revision = -1;
	
	filename = path;
	filename += "\\teraterm\\ttpdlg\\svnversion.h";


	// read current file
	ret = csf.Open(filename, CFile::modeRead);
	if (ret == TRUE) {
		csf.SeekToBegin();
		csf.ReadString(cs);
		csf.Close();

		ret = sscanf_s(cs, "#define SVNVERSION %d", &file_revision);
	}

	// compare revisions
	if (file_revision >= revision) {
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
	int nRetCode = 0;
	int revision = -1;
	char path[MAX_PATH * 2];
	int i, len;

	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
		return 1;
	}

	GetModuleFileName(::GetModuleHandle(NULL), path, sizeof(path));
	len = (int)strlen(path);
	for (i=len; i>=0; i--) {
		if (path[i] == '\\') {
			break;
		}
		path[i] = '\0';
	}
	SetCurrentDirectory(path); // teraterm\debug or teraterm\release
	SetCurrentDirectory("..\\..\\"); // top of source tree
	GetCurrentDirectory(sizeof(path), path);

	revision = get_svn_revision(path);

	if (!write_svn_revesion(path, revision)) {
		return 1;
	}

	return 0;
}
