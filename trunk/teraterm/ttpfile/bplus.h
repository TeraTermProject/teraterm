/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTFILE.DLL, B-Plus protocol */

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
void BPInit
  (PFileVar fv, PBPVar bv, PComVar cv, PTTSet ts);
void BPTimeOutProc(PFileVar fv, PBPVar bv, PComVar cv);
BOOL BPParse(PFileVar fv, PBPVar bv, PComVar cv);
void BPCancel(PBPVar bv);

#ifdef __cplusplus
}
#endif
