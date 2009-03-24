/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTFILE.DLL, Quick-VAN protocol */

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
void QVInit
  (PFileVar fv, PQVVar qv, PComVar cv, PTTSet ts);
void QVCancel(PFileVar fv, PQVVar qv, PComVar cv);
void QVTimeOutProc(PFileVar fv, PQVVar qv, PComVar cv);
BOOL QVReadPacket(PFileVar fv, PQVVar qv, PComVar cv);
BOOL QVSendPacket(PFileVar fv, PQVVar qv, PComVar cv);

#ifdef __cplusplus
}
#endif
