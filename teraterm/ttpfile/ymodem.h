/* Tera Term
 Copyright(C) 2008 TeraTerm Project
 All rights reserved. */

/* TTFILE.DLL, YMODEM protocol */

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
void YInit
  (PFileVar fv, PYVar yv, PComVar cv, PTTSet ts);
void YCancel(PFileVar fv, PYVar yv, PComVar cv);
void YTimeOutProc(PFileVar fv, PYVar yv, PComVar cv);
BOOL YReadPacket(PFileVar fv, PYVar yv, PComVar cv);
BOOL YSendPacket(PFileVar fv, PYVar yv, PComVar cv);

#ifdef __cplusplus
}
#endif
