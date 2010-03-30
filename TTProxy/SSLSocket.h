#ifndef _YEBISOCKS_SSLSOCKET_H_
#define _YEBISOCKS_SSLSOCKET_H_

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#include "SSLLIB.h"

#define IDS_UNABLE_TO_GET_ISSUER_CERT   200
#define IDS_UNABLE_TO_GET_CRL           201
#define IDS_UNABLE_TO_DECRYPT_CERT_SIGNATURE 202
#define IDS_UNABLE_TO_DECRYPT_CRL_SIGNATURE 203
#define IDS_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY 204
#define IDS_CERT_SIGNATURE_FAILURE      205
#define IDS_CRL_SIGNATURE_FAILURE       206
#define IDS_CERT_NOT_YET_VALID          207
#define IDS_CERT_HAS_EXPIRED            208
#define IDS_CRL_NOT_YET_VALID           209
#define IDS_CRL_HAS_EXPIRED             210
#define IDS_ERROR_IN_CERT_NOT_BEFORE_FIELD 211
#define IDS_ERROR_IN_CERT_NOT_AFTER_FIELD 212
#define IDS_ERROR_IN_CRL_LAST_UPDATE_FIELD 213
#define IDS_ERROR_IN_CRL_NEXT_UPDATE_FIELD 214
#define IDS_OUT_OF_MEM                  215
#define IDS_DEPTH_ZERO_SELF_SIGNED_CERT 216
#define IDS_SELF_SIGNED_CERT_IN_CHAIN   217
#define IDS_UNABLE_TO_GET_ISSUER_CERT_LOCALLY 218
#define IDS_UNABLE_TO_VERIFY_LEAF_SIGNATURE 219
#define IDS_CERT_CHAIN_TOO_LONG         220
#define IDS_CERT_REVOKED                221
#define IDS_INVALID_CA                  222
#define IDS_PATH_LENGTH_EXCEEDED        223
#define IDS_INVALID_PURPOSE             224
#define IDS_CERT_UNTRUSTED              225
#define IDS_CERT_REJECTED               226
#define IDS_SUBJECT_ISSUER_MISMATCH     227
#define IDS_AKID_SKID_MISMATCH          228
#define IDS_AKID_ISSUER_SERIAL_MISMATCH 229
#define IDS_KEYUSAGE_NO_CERTSIGN        230
#define IDS_APPLICATION_VERIFICATION    231
#define IDS_UNMATCH_COMMON_NAME         232
#define IDS_UNABLE_TO_GET_COMMON_NAME   233

class SSLSocket {
private:
    class SSLContext {
        friend class SSLSocket;
    private:
        SSL_CTX* ctx;

    private:
        SSLContext():ctx(NULL) {
            SSL_library_init(); 
            SSL_load_error_strings();
            ctx = SSL_CTX_new(SSLv23_client_method());
            SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
        }
    public:
        ~SSLContext() {
            release();
        }
        void release() {
            if (ctx != NULL) {
                SSL_CTX_free(ctx);
                ctx = NULL;
            }
        }
    };
    static SSLContext& context() {
        static SSLContext instance;
        return instance;
    }
    static HMODULE& LIBEAY32() {
        static HMODULE module = NULL;
        return module;
    }
    static HMODULE& SSLEAY32() {
        static HMODULE module = NULL;
        return module;
    }
public:
    static bool init() {
        LIBEAY32() = ::LoadLibrary("LIBEAY32");
        SSLEAY32() = ::LoadLibrary("SSLEAY32");
        if (LIBEAY32() == NULL || SSLEAY32() == NULL) {
            if (LIBEAY32() != NULL) {
                ::FreeLibrary(LIBEAY32());
                LIBEAY32() = NULL;
            }
            if (SSLEAY32() != NULL) {
                ::FreeLibrary(SSLEAY32());
                SSLEAY32() = NULL;
            }
            return false;
        }
        context();
        return true;
    }
    static void exit() {
        if (LIBEAY32() != NULL && SSLEAY32() != NULL) {
            context().release();
            ::FreeLibrary(LIBEAY32());
            LIBEAY32() = NULL;
            ::FreeLibrary(SSLEAY32());
            SSLEAY32() = NULL;
        }
    }
    static bool isEnabled() {
        return LIBEAY32() != NULL && SSLEAY32() != NULL;
    }

    static bool addCertFile(const char* filename) {
        return SSL_CTX_load_verify_locations(context().ctx, filename, NULL) != 0;
    }
    static bool addCertDirectory(const char* dirname) {
        return SSL_CTX_load_verify_locations(context().ctx, NULL, dirname) != 0;
    }
    static bool addCert(const char* filename, const char* dirname) {
        return SSL_CTX_load_verify_locations(context().ctx, filename, dirname) != 0;
    }

private:
    static int toLowerCase(int ch) {
        return 'A' <= ch && ch <= 'Z' ? ch - 'A' + 'a' : ch;
    }
    static bool ssl_match_cert_ident(const char* ident, int ilen, const char* hostname) {
        const char* s1 = ident;
        const char* s2 = hostname;
        const char* s1_end = s1 + ilen;
        const char* s2_end = s2 + strlen(s2);
        while (s1 < s1_end && s2 < s2_end) {
            if (*s1 == '*' && s1 + 1 < s1_end && *(s1 + 1) == '.') {
                s1++;
                while (s2 < s2_end && *s2 != '.')
                    s2++;
                if (*s2 != '.')
                    return false;
            }else{
                if (::IsDBCSLeadByte(*s1)) {
                    if (*s1 != *s2 || s1 + 1 >= s1_end || s2 + 1 >= s2_end || *(s1 + 1) != *(s2 + 1))
                        return false;
                    s1++;
                    s2++;
                }else{
                    if (toLowerCase(*s1) != toLowerCase(*s2))
                        return false;
                }
            }
            s1++;
            s2++;
        }
        return true;
    }

    static int print_debugw32(const char *str, size_t len, void *u) {
        char* buf = (char*) alloca(len + 1);
        memcpy(buf, str, len);
        buf[len] = '\0';
        OutputDebugString(buf);
        return len;
    }
private:
    SSL* ssl;
    long verify_result;
public:
    SSLSocket():ssl(NULL), verify_result(X509_V_ERR_OUT_OF_MEM) {
    }
    ~SSLSocket() {
        close();
    }
    bool connect(SOCKET s, const char* hostname) {
        close();
        verify_result = IDS_APPLICATION_VERIFICATION;
        ssl = SSL_new(context().ctx);
        if (ssl != NULL) {
            SSL_set_fd(ssl, s);
            int ret;
            do {
                ret = SSL_connect(ssl);
                if (SSL_get_error(ssl, ret) != SSL_ERROR_WANT_READ)
                    break;
                fd_set ifd;
                fd_set efd;
                FD_ZERO(&ifd);
                FD_ZERO(&efd);
                FD_SET(s, &ifd);
                FD_SET(s, &efd);
                if (select((int) (s + 1), &ifd, NULL, &efd, NULL) == SOCKET_ERROR)
                    break;
                if (!FD_ISSET(s, &ifd))
                    break;
            }while (ret < 0);
            if (ret == 1) {
                X509* x509 = SSL_get_peer_certificate(ssl);
                if (x509 != NULL) {
                    bool match = false;
                    /* SSL‚Å‚Ì”FØ */
                    int result = SSL_get_verify_result(ssl);
                    if (result == X509_V_OK) {
                        verify_result = 0;
                        int id = X509_get_ext_by_NID(x509, NID_subject_alt_name, -1);
                        if (id >= 0) {
                            X509_EXTENSION* ex = X509_get_ext(x509, id);
                            STACK_OF(GENERAL_NAME)* alt = (STACK_OF(GENERAL_NAME)*) X509V3_EXT_d2i(ex);
                            if (alt != NULL) {
                                int i;
                                int count = 0;
                                int n = sk_GENERAL_NAME_num(alt);
                                for (i = 0; i < n; i++) {
                                    GENERAL_NAME* gn = sk_GENERAL_NAME_value(alt, i);
                                    if (gn->type == GEN_DNS) {
                                        char *sn = (char*) ASN1_STRING_data(gn->d.ia5);
                                        int sl = ASN1_STRING_length(gn->d.ia5);
                                        count++;
                                        if (ssl_match_cert_ident(sn, sl, hostname))
                                            break;
                                    }
                                }
#if (OPENSSL_VERSION_MAJOR < 1)
                                X509V3_EXT_METHOD* method = X509V3_EXT_get(ex);
#else
                                const X509V3_EXT_METHOD* method = X509V3_EXT_get(ex);
#endif
                                sk_GENERAL_NAME_free(alt);
                                if (i < n)
                                    match = true;
                                else if (count > 0)
                                    verify_result = IDS_UNMATCH_COMMON_NAME; /* common name‚ªˆê’v‚µ‚È‚©‚Á‚½ */
                            }
                        }
                        if (!match && verify_result == 0) {
                            X509_NAME* xn = X509_get_subject_name(x509);
                            char buf[1024];
                            if (X509_NAME_get_text_by_NID(xn, NID_commonName, buf, sizeof buf) == -1)
                                verify_result = IDS_UNABLE_TO_GET_COMMON_NAME; /* common name‚ÌŽæ“¾‚ÉŽ¸”s‚µ‚½ */
                            else if (!ssl_match_cert_ident(buf, strlen(buf), hostname))
                                verify_result = IDS_UNMATCH_COMMON_NAME; /* common name‚ªˆê’v‚µ‚È‚©‚Á‚½ */
                            else
                                match = true;
                        }
                    }else{
                        static const long table[] = {
                            X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT, IDS_UNABLE_TO_GET_ISSUER_CERT,
                            X509_V_ERR_UNABLE_TO_GET_CRL, IDS_UNABLE_TO_GET_CRL,
                            X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE, IDS_UNABLE_TO_DECRYPT_CERT_SIGNATURE,
                            X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE, IDS_UNABLE_TO_DECRYPT_CRL_SIGNATURE,
                            X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY, IDS_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY,
                            X509_V_ERR_CERT_SIGNATURE_FAILURE, IDS_CERT_SIGNATURE_FAILURE,
                            X509_V_ERR_CRL_SIGNATURE_FAILURE, IDS_CRL_SIGNATURE_FAILURE,
                            X509_V_ERR_CERT_NOT_YET_VALID, IDS_CERT_NOT_YET_VALID,
                            X509_V_ERR_CERT_HAS_EXPIRED, IDS_CERT_HAS_EXPIRED,
                            X509_V_ERR_CRL_NOT_YET_VALID, IDS_CRL_NOT_YET_VALID,
                            X509_V_ERR_CRL_HAS_EXPIRED, IDS_CRL_HAS_EXPIRED,
                            X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD, IDS_ERROR_IN_CERT_NOT_BEFORE_FIELD,
                            X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD, IDS_ERROR_IN_CERT_NOT_AFTER_FIELD,
                            X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD, IDS_ERROR_IN_CRL_LAST_UPDATE_FIELD,
                            X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD, IDS_ERROR_IN_CRL_NEXT_UPDATE_FIELD,
                            X509_V_ERR_OUT_OF_MEM, IDS_OUT_OF_MEM,
                            X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT, IDS_DEPTH_ZERO_SELF_SIGNED_CERT,
                            X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN, IDS_SELF_SIGNED_CERT_IN_CHAIN,
                            X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY, IDS_UNABLE_TO_GET_ISSUER_CERT_LOCALLY,
                            X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE, IDS_UNABLE_TO_VERIFY_LEAF_SIGNATURE,
                            X509_V_ERR_CERT_CHAIN_TOO_LONG, IDS_CERT_CHAIN_TOO_LONG,
                            X509_V_ERR_CERT_REVOKED, IDS_CERT_REVOKED,
                            X509_V_ERR_INVALID_CA, IDS_INVALID_CA,
                            X509_V_ERR_PATH_LENGTH_EXCEEDED, IDS_PATH_LENGTH_EXCEEDED,
                            X509_V_ERR_INVALID_PURPOSE, IDS_INVALID_PURPOSE,
                            X509_V_ERR_CERT_UNTRUSTED, IDS_CERT_UNTRUSTED,
                            X509_V_ERR_CERT_REJECTED, IDS_CERT_REJECTED,
                            X509_V_ERR_SUBJECT_ISSUER_MISMATCH, IDS_SUBJECT_ISSUER_MISMATCH,
                            X509_V_ERR_AKID_SKID_MISMATCH, IDS_AKID_SKID_MISMATCH,
                            X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH, IDS_AKID_ISSUER_SERIAL_MISMATCH,
                            X509_V_ERR_KEYUSAGE_NO_CERTSIGN, IDS_KEYUSAGE_NO_CERTSIGN,
                            X509_V_ERR_APPLICATION_VERIFICATION, IDS_APPLICATION_VERIFICATION,
                        };
                        for (int i = 0; i < countof(table); i += 2) {
                            if (table[i] == result) {
                                verify_result = table[i + 1];
                                break;
                            }
                        }
                    }
                    X509_free(x509);
                    if (match)
                        return true;
                }
            }
            SSL_free(ssl);
            ssl = NULL;
        }
        return false;
    }
    void close() {
        if (ssl != NULL) {
            SSL_shutdown(ssl);
            SSL_free(ssl); 
            ssl = NULL;
        }
    }
    char* get_cert_text()const {
        char* text = NULL;
        X509* x509 = SSL_get_peer_certificate(ssl); 
        if (x509 != NULL) {
            BIO* bp = BIO_new(BIO_s_mem());
            if (bp != NULL) {
                if (PEM_write_bio_X509(bp, x509)) {
                    BUF_MEM* buf;
                    BIO_get_mem_ptr(bp, &buf);
                    text = (char*) OPENSSL_malloc(buf->length + 1);
                    if (text != NULL) {
                        memcpy(text, buf->data, buf->length);
                        text[buf->length] = '\0';
                    }
                }
                BIO_free(bp);
            }
            X509_free(x509);
        }
        return text;
    }
    size_t write(const void* buffer, size_t length) {
        return ssl != NULL ? SSL_write(ssl, buffer, length) : -1;
    }
    size_t read(void* buffer, size_t length) {
        return ssl != NULL ? SSL_read(ssl, buffer, length) : -1;
    }
    long get_verify_result()const {
        return verify_result;
    }
};

#endif//_YEBISOCKS_SSLSOCKET_H_
