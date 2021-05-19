/*
 * Visual Studio 2008 or earlier doesn't have stdint.h
 */

#if (_MSC_VER < 1300)
	typedef unsigned char   uint8_t;
	typedef unsigned short  uint32_t;
#else
	typedef unsigned __int8   uint8_t;
	typedef unsigned __int32  uint32_t;
#endif
typedef unsigned __int64  uint64_t;

#define UINT32_MAX   _UI32_MAX

#define UINT32_C(val) val##ui32
#define UINT64_C(val) val##ui64
