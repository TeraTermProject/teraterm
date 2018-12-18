

#ifdef __cplusplus
extern "C" {
#endif

WORD TTLVar2Clipb();
WORD TTLClipb2Var();
WORD TTLFilenameBox();
WORD TTLDirnameBox();

#define IdMsgBox 1
#define IdYesNoBox 2
#define IdStatusBox 3
#define IdListBox 4
#define LISTBOX_ITEM_NUM 10

int MessageCommand(int BoxId, LPWORD Err);
WORD TTLListBox();
WORD TTLMessageBox();
WORD TTLGetPassword();
WORD TTLInputBox(BOOL Paswd);

#ifdef __cplusplus
}
#endif
