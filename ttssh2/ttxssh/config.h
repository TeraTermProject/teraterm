// ドラフトでなくなった場合に ifdef を削除するために使用する。
// コンパイルにおいて on/off するために利用すると、TERATERM.INIに
// 保存するときの記号がずれてしまうのでやってはいけない。

// Camellia support draft
// http://tools.ietf.org/html/draft-kanno-secsh-camellia-02
// https://bugzilla.mindrot.org/show_bug.cgi?id=1340
#undef WITH_CAMELLIA_DRAFT
#undef WITH_CAMELLIA_PRIVATE

#if defined(WITH_CAMELLIA_PRIVATE) && !defined(WITH_CAMELLIA_DRAFT)
#define WITH_CAMELLIA_DRAFT
#endif
