/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTFILE.DLL, ZMODEM protocol */

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
void ZInit
  (PFileVar fv, PZVar zv, PComVar cv, PTTSet ts);
void ZTimeOutProc(PFileVar fv, PZVar zv, PComVar cv);
BOOL ZParse(PFileVar fv, PZVar zv, PComVar cv);
void ZCancel(PZVar zv);

#ifdef __cplusplus
}
#endif
