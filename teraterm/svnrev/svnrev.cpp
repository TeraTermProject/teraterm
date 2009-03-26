#include "stdafx.h"
#include "svnrev.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;
	int revision = -1;

	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		nRetCode = 1;
	}
	else
	{
		BOOL ret;
		CStdioFile csf;
		CString cs;
		int format = -1;
		
		// [root dir]\.svn\entries
		ret = csf.Open("..\\..\\.svn\\entries", CFile::modeRead);
		if (ret == FALSE) {
			nRetCode = 1;
		}
		else {
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
		}
	}

	if (nRetCode == 0) {
		printf("#define SVNVERSION %d\n", revision);
	}
	else {
		printf("#undef SVNVERSION\n");
	}

	return nRetCode;
}
