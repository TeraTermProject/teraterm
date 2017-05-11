#ifndef BUFFER_H
#define BUFFER_H

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <zlib.h>

typedef struct buffer {
	char *buf;      /* バッファの先頭ポインタ。realloc()により変動する。*/
	size_t offset;     /* 現在の読み出し位置 */
	size_t maxlen;     /* バッファの最大サイズ */
	size_t len;        /* バッファに含まれる有効なデータサイズ */
} buffer_t;

/* buffer_t.buf の拡張の上限値 (16MB) */
#define BUFFER_SIZE_MAX 0x1000000
/* buffer_t.buf の拡張時に追加で確保する量 (32KB) */
#define BUFFER_INCREASE_MARGIN (32*1024)

void buffer_clear(buffer_t *buf);
buffer_t *buffer_init(void);
void buffer_free(buffer_t *buf);
void *buffer_append_space(buffer_t * buf, size_t size);
int buffer_append(buffer_t *buf, char *ptr, int size);
int buffer_append_length(buffer_t *msg, char *ptr, int size);
void buffer_put_raw(buffer_t *msg, char *ptr, int size);
char *buffer_get_string(char **data_ptr, int *buflen_ptr);
void buffer_put_string(buffer_t *msg, char *ptr, int size);
void buffer_put_cstring(buffer_t *msg, char *ptr);
void buffer_put_char(buffer_t *msg, int value);
void buffer_put_padding(buffer_t *msg, int size);
void buffer_put_int(buffer_t *msg, int value);
int buffer_len(buffer_t *msg);
char *buffer_ptr(buffer_t *msg);
void buffer_put_bignum(buffer_t *buffer, BIGNUM *value);
void buffer_put_bignum2(buffer_t *msg, BIGNUM *value);
void buffer_get_bignum2(char **data, BIGNUM *value);
void buffer_get_bignum2_msg(buffer_t *msg, BIGNUM *value);
void buffer_get_bignum_SECSH(buffer_t *buffer, BIGNUM *value);
void buffer_put_ecpoint(buffer_t *msg, const EC_GROUP *curve, const EC_POINT *point);
void buffer_get_ecpoint(char **data, const EC_GROUP *curve, EC_POINT *point);
void buffer_get_ecpoint_msg(buffer_t *msg, const EC_GROUP *curve, EC_POINT *point);
char *buffer_tail_ptr(buffer_t *msg);
int buffer_overflow_verify(buffer_t *msg, int len);
void buffer_consume(buffer_t *buf, size_t shift_byte);
void buffer_consume_end(buffer_t *buf, size_t shift_byte);
int buffer_compress(z_stream *zstream, char *payload, int len, buffer_t *compbuf);
int buffer_decompress(z_stream *zstream, char *payload, int len, buffer_t *compbuf);
int buffer_get_ret(buffer_t *msg, void *buf, size_t len);
int buffer_get_int_ret(unsigned int *ret, buffer_t *msg);
unsigned int buffer_get_int(buffer_t *msg);
int buffer_get_char_ret(char *ret, buffer_t *msg);
int buffer_get_char(buffer_t *msg);
void buffer_rewind(buffer_t *buf);
void *buffer_get_string_msg(buffer_t *msg, int *buflen_ptr);
int buffer_remain_len(buffer_t *msg);

#endif				/* BUFFER_H */
