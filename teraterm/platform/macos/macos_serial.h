/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS serial port (COM port) abstraction using IOKit.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void* TTMacSerial;

/* Enumerate available serial ports. Returns count, fills names array. */
int tt_mac_serial_enumerate(char names[][256], int max_ports);

/* Open/close serial port */
TTMacSerial tt_mac_serial_open(const char* device_path);
void tt_mac_serial_close(TTMacSerial port);

/* Configuration */
int tt_mac_serial_set_baud(TTMacSerial port, int baud_rate);
int tt_mac_serial_set_params(TTMacSerial port, int data_bits, int stop_bits, int parity);
int tt_mac_serial_set_flow_control(TTMacSerial port, int flow);

/* I/O */
int tt_mac_serial_read(TTMacSerial port, void* buffer, int size);
int tt_mac_serial_write(TTMacSerial port, const void* buffer, int size);
int tt_mac_serial_bytes_available(TTMacSerial port);

/* Control signals */
int tt_mac_serial_set_dtr(TTMacSerial port, int state);
int tt_mac_serial_set_rts(TTMacSerial port, int state);
int tt_mac_serial_get_cts(TTMacSerial port);
int tt_mac_serial_get_dsr(TTMacSerial port);
int tt_mac_serial_send_break(TTMacSerial port);

#ifdef __cplusplus
}
#endif
