/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTFILE.DLL, XMODEM protocol */

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
void XInit
  (PFileVar fv, PXVar xv, PComVar cv, PTTSet ts);
void XCancel(PFileVar fv, PXVar xv, PComVar cv);
void XTimeOutProc(PFileVar fv, PXVar xv, PComVar cv);
BOOL XReadPacket(PFileVar fv, PXVar xv, PComVar cv);
BOOL XSendPacket(PFileVar fv, PXVar xv, PComVar cv);

#ifdef __cplusplus
}
#endif
