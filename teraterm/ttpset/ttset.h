#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DllExport)
#define DllExport __declspec(dllimport)
#endif

DllExport int WINAPI SerialPortConfconvertId2Str(enum serial_port_conf type, WORD id, PCHAR str, int strlen);

#ifdef __cplusplus
}
#endif
