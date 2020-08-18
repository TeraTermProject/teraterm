
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CheckEOLData_st CheckEOLData_t;

typedef enum {
	CheckEOLNoOutput = 0x00,	// âΩÇ‡èoóÕÇµÇ»Ç¢
	CheckEOLOutputEOL = 0x01,	// EOLÇèoóÕÇ∑ÇÈ
	CheckEOLOutputChar = 0x02,	// ÇªÇÃÇ‹Ç‹èoóÕÇ∑ÇÈ
} CheckEOLRet;

CheckEOLData_t *CheckEOLCreate(void);
void CheckEOLDestroy(CheckEOLData_t *self);
void CheckEOLClear(CheckEOLData_t *self);
CheckEOLRet CheckEOLCheck(CheckEOLData_t *self, unsigned int u32);

#ifdef __cplusplus
}
#endif
