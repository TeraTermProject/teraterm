/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS serial port implementation using POSIX termios and IOKit.
 */

#include "macos_serial.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <glob.h>

typedef struct {
    int fd;
    struct termios original_attrs;
} TTMacSerialImpl;

int tt_mac_serial_enumerate(char names[][256], int max_ports) {
    int count = 0;
    glob_t g;

    /* macOS serial devices: USB, Bluetooth, built-in */
    const char* patterns[] = {
        "/dev/tty.usbserial*",
        "/dev/tty.usbmodem*",
        "/dev/tty.Bluetooth*",
        "/dev/tty.serial*",
        NULL
    };

    for (int i = 0; patterns[i] && count < max_ports; i++) {
        if (glob(patterns[i], 0, NULL, &g) == 0) {
            for (size_t j = 0; j < g.gl_pathc && count < max_ports; j++) {
                strncpy(names[count], g.gl_pathv[j], 255);
                names[count][255] = '\0';
                count++;
            }
            globfree(&g);
        }
    }

    return count;
}

TTMacSerial tt_mac_serial_open(const char* device_path) {
    if (!device_path) return NULL;

    int fd = open(device_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return NULL;

    /* Ensure exclusive access */
    if (ioctl(fd, TIOCEXCL) == -1) {
        close(fd);
        return NULL;
    }

    /* Clear non-blocking for normal operation */
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    TTMacSerialImpl* impl = (TTMacSerialImpl*)calloc(1, sizeof(TTMacSerialImpl));
    if (!impl) {
        close(fd);
        return NULL;
    }

    impl->fd = fd;

    /* Save original termios */
    tcgetattr(fd, &impl->original_attrs);

    /* Configure raw mode */
    struct termios options;
    tcgetattr(fd, &options);
    cfmakeraw(&options);
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10;  /* 1 second timeout */
    tcsetattr(fd, TCSANOW, &options);

    return (TTMacSerial)impl;
}

void tt_mac_serial_close(TTMacSerial port) {
    if (!port) return;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;

    /* Restore original settings */
    tcsetattr(impl->fd, TCSANOW, &impl->original_attrs);
    close(impl->fd);
    free(impl);
}

static speed_t baud_to_speed(int baud) {
    switch (baud) {
        case 300:    return B300;
        case 600:    return B600;
        case 1200:   return B1200;
        case 2400:   return B2400;
        case 4800:   return B4800;
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        default:     return B9600;
    }
}

int tt_mac_serial_set_baud(TTMacSerial port, int baud_rate) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;

    struct termios options;
    tcgetattr(impl->fd, &options);
    speed_t speed = baud_to_speed(baud_rate);
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);
    return tcsetattr(impl->fd, TCSANOW, &options);
}

int tt_mac_serial_set_params(TTMacSerial port, int data_bits, int stop_bits, int parity) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;

    struct termios options;
    tcgetattr(impl->fd, &options);

    /* Data bits */
    options.c_cflag &= ~CSIZE;
    switch (data_bits) {
        case 5: options.c_cflag |= CS5; break;
        case 6: options.c_cflag |= CS6; break;
        case 7: options.c_cflag |= CS7; break;
        default: options.c_cflag |= CS8; break;
    }

    /* Stop bits */
    if (stop_bits == 2)
        options.c_cflag |= CSTOPB;
    else
        options.c_cflag &= ~CSTOPB;

    /* Parity: 0=none, 1=odd, 2=even */
    options.c_cflag &= ~(PARENB | PARODD);
    if (parity == 1) {
        options.c_cflag |= PARENB | PARODD;
    } else if (parity == 2) {
        options.c_cflag |= PARENB;
    }

    return tcsetattr(impl->fd, TCSANOW, &options);
}

int tt_mac_serial_set_flow_control(TTMacSerial port, int flow) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;

    struct termios options;
    tcgetattr(impl->fd, &options);

    /* 0=none, 1=xon/xoff, 2=rts/cts */
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_cflag &= ~CRTSCTS;

    if (flow == 1) {
        options.c_iflag |= IXON | IXOFF;
    } else if (flow == 2) {
        options.c_cflag |= CRTSCTS;
    }

    return tcsetattr(impl->fd, TCSANOW, &options);
}

int tt_mac_serial_read(TTMacSerial port, void* buffer, int size) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;
    return (int)read(impl->fd, buffer, size);
}

int tt_mac_serial_write(TTMacSerial port, const void* buffer, int size) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;
    return (int)write(impl->fd, buffer, size);
}

int tt_mac_serial_bytes_available(TTMacSerial port) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;
    int bytes = 0;
    ioctl(impl->fd, FIONREAD, &bytes);
    return bytes;
}

int tt_mac_serial_set_dtr(TTMacSerial port, int state) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;
    int flag = TIOCM_DTR;
    return ioctl(impl->fd, state ? TIOCMBIS : TIOCMBIC, &flag);
}

int tt_mac_serial_set_rts(TTMacSerial port, int state) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;
    int flag = TIOCM_RTS;
    return ioctl(impl->fd, state ? TIOCMBIS : TIOCMBIC, &flag);
}

int tt_mac_serial_get_cts(TTMacSerial port) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;
    int status = 0;
    ioctl(impl->fd, TIOCMGET, &status);
    return (status & TIOCM_CTS) ? 1 : 0;
}

int tt_mac_serial_get_dsr(TTMacSerial port) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;
    int status = 0;
    ioctl(impl->fd, TIOCMGET, &status);
    return (status & TIOCM_DSR) ? 1 : 0;
}

int tt_mac_serial_send_break(TTMacSerial port) {
    if (!port) return -1;
    TTMacSerialImpl* impl = (TTMacSerialImpl*)port;
    return tcsendbreak(impl->fd, 0);
}
