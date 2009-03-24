#ifndef _SSLLIB_h_
#define _SSLLIB_h_

#define DECLARE_MODULE_API(module, rettype, apiname, arglist, args) \
rettype apiname arglist {                                     \
    static rettype (*func) arglist =                          \
       (rettype (*) arglist)                                  \
       GetProcAddress(GetModuleHandle(#module), #apiname);    \
    return (*func) args;                                      \
}
#define DECLARE_MODULE_API_v(module, apiname, arglist, args) \
    void apiname arglist {                                   \
    static void (*func) arglist =                            \
       (void (*) arglist)                                    \
       GetProcAddress(GetModuleHandle(#module), #apiname);   \
    (*func) args;                                            \
}

#define DECLARE_LIBEAY32_API(rettype, apiname, arglist, args) DECLARE_MODULE_API(LIBEAY32, rettype, apiname, arglist, args)
#define DECLARE_LIBEAY32_API_v(apiname, arglist, args) DECLARE_MODULE_API_v(LIBEAY32, apiname, arglist, args)
#define DECLARE_SSLEAY32_API(rettype, apiname, arglist, args) DECLARE_MODULE_API(SSLEAY32, rettype, apiname, arglist, args)
#define DECLARE_SSLEAY32_API_v(apiname, arglist, args) DECLARE_MODULE_API_v(SSLEAY32, apiname, arglist, args)

DECLARE_LIBEAY32_API(int, X509_NAME_get_text_by_NID, (X509_NAME *name, int nid, char *buf,int len), (name, nid, buf, len))
DECLARE_LIBEAY32_API_v(X509_free, (X509 *a), (a))
DECLARE_LIBEAY32_API(X509_EXTENSION*, X509_get_ext, (X509 *x, int loc), (x, loc))
DECLARE_LIBEAY32_API(int, X509_get_ext_by_NID, (X509 *x, int nid, int lastpos), (x, nid, lastpos))
DECLARE_LIBEAY32_API(X509_NAME*, X509_get_subject_name, (X509 *a), (a))
DECLARE_LIBEAY32_API_v(sk_free, (STACK *x), (x))
DECLARE_LIBEAY32_API(X509V3_EXT_METHOD*, X509V3_EXT_get, (X509_EXTENSION *ext), (ext))
DECLARE_LIBEAY32_API(void*, X509V3_EXT_d2i, (X509_EXTENSION *ext), (ext))
DECLARE_LIBEAY32_API(char*, sk_value, (const STACK *a1, int a2), (a1, a2))
DECLARE_LIBEAY32_API(int, sk_num, (const STACK *x), (x))
DECLARE_LIBEAY32_API(int, ASN1_STRING_length, (ASN1_STRING *x), (x))
DECLARE_LIBEAY32_API(unsigned char *, ASN1_STRING_data, (ASN1_STRING *x), (x))

DECLARE_SSLEAY32_API(long, SSL_CTX_ctrl, (SSL_CTX *ctx,int cmd, long larg, void *parg), (ctx, cmd, larg, parg))
DECLARE_SSLEAY32_API_v(SSL_CTX_free, (SSL_CTX *ctx), (ctx))
DECLARE_SSLEAY32_API(SSL_CTX *, SSL_CTX_new, (SSL_METHOD *meth), (meth))
DECLARE_SSLEAY32_API(int, SSL_connect, (SSL *ssl), (ssl))
DECLARE_SSLEAY32_API_v(SSL_free, (SSL *ssl), (ssl))
DECLARE_SSLEAY32_API(int, SSL_get_error, (const SSL *s,int ret_code), (s,ret_code))
DECLARE_SSLEAY32_API(X509 *, SSL_get_peer_certificate, (const SSL *s), (s))
DECLARE_SSLEAY32_API_v(SSL_load_error_strings, (void ), ())
DECLARE_SSLEAY32_API(SSL *, SSL_new, (SSL_CTX *ctx), (ctx))
DECLARE_SSLEAY32_API(int, SSL_read, (SSL *ssl,void *buf,int num), (ssl, buf, num))
DECLARE_SSLEAY32_API(int, SSL_set_fd, (SSL *s, int fd), (s, fd))
DECLARE_SSLEAY32_API(int, SSL_shutdown, (SSL *s), (s))
DECLARE_SSLEAY32_API(int, SSL_write, (SSL *ssl,const void *buf,int num), (ssl, buf, num))
DECLARE_SSLEAY32_API(SSL_METHOD *, SSLv23_client_method, (void), ())
DECLARE_SSLEAY32_API(int, SSL_CTX_load_verify_locations, (SSL_CTX *ctx, const char *CAfile, const char *CApath), (ctx, CAfile, CApath))
DECLARE_SSLEAY32_API(long, SSL_get_verify_result, (const SSL *ssl), (ssl))
DECLARE_SSLEAY32_API(int, SSL_library_init, (void), ())

#endif//_SSLLIB_h_