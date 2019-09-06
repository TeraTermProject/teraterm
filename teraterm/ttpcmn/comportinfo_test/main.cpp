#include <stdio.h>
#include <tchar.h>
#include <locale.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "comportinfo.h"

int main(int, char *)
{
	printf("start\n");

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	setlocale(LC_ALL, "");

	int comPortCount;
	ComPortInfo_t *infos = ComPortInfoGet(&comPortCount, NULL);
	for(int i=0; i< comPortCount; i++) {
		ComPortInfo_t *p = &infos[i];
		//const WORD port_no = ComPortTable[i];
		wchar_t *port = p->port_name;
		printf("========\n"
			   "%ls:\n", port);
		free(port);
		wchar_t *friendly = p->friendly_name;
		wchar_t *desc = p->property;
		if (friendly != NULL) {
			printf("-----\n"
				   "FRIENDLY\n"
				   "%ls\n", friendly);
			free(friendly);
		}
		if(desc != NULL) {
			printf("-----\n"
				   "DESC\n"
				   "%ls\n", desc);
			free(desc);
		}
	}
	free(infos);

	printf("end\n");
	return 0;
}
