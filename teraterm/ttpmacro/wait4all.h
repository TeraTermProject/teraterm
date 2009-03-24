/*
 * wait4all.h
 *
 */
#ifndef _WAIT4ALL_H_
#define _WAIT4ALL_H_

#ifdef __cplusplus
extern "C" {
#endif

// TODO: ˆÈ‰º‚Ì’è‹`‚Í‹¤’Êƒwƒbƒ_‚©‚çinclude‚·‚×‚«
#undef MAXNWIN
#define MAXNWIN 50
#undef RingBufSize
#define RingBufSize (4096*4)

int is_wait4all_enabled(void);
int register_macro_window(HWND hwnd);
int unregister_macro_window(HWND hwnd);
void get_macro_active_info(int *num, int *index);
int get_macro_active_num(void);
void put_macro_1byte(BYTE b);
int read_macro_1byte(int index, LPBYTE b);

extern int macro_shmem_index;

#ifdef __cplusplus
}
#endif

#endif

