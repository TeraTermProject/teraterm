// ドラフトでなくなった場合に ifdef を削除するために使用する。
// コンパイルにおいて on/off するために利用すると、TERATERM.INIに
// 保存するときの記号がずれてしまうのでやってはいけない。

// HMAC-SHA2 draft
// http://tools.ietf.org/html/draft-dbider-sha2-mac-for-ssh-02
#undef WITH_HMAC_SHA2_DRAFT

// Camellia support draft
// http://tools.ietf.org/html/draft-kanno-secsh-camellia-02
#undef WITH_CAMELLIA_DRAFT
#undef WITH_CAMELLIA_PRIVATE

#if defined(WITH_CAMELLIA_PRIVATE) && !defined(WITH_CAMELLIA_DRAFT)
#define WITH_CAMELLIA_DRAFT
#endif
