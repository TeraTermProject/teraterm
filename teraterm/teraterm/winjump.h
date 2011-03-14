#ifdef __cplusplus
extern "C" {
#endif

void clear_jumplist(void);
void add_session_to_jumplist(const char * const sessionname, char *inifile);
void remove_session_from_jumplist(const char * const sessionname);

#ifdef __cplusplus
}
#endif
