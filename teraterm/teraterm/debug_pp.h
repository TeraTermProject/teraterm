

#include "tmfc.h"

class CDebugPropPage : public TTCPropertyPage
{
public:
	CDebugPropPage(HINSTANCE inst, TTCPropertySheet *sheet);
	virtual ~CDebugPropPage();
private:
	void OnInitDialog();
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnOK();
};
