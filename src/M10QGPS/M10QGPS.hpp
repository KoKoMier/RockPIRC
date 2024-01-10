#pragma once
#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <iomanip>
#include <asm-generic/ioctls.h>
#include <asm/termbits.h>

class M10QGPS
{
public:
    M10QGPS(const char *UartDevice)
    {
        GPSDevice = UartDevice;
        GPSUart_fd = open(UartDevice, O_RDWR | O_NOCTTY | O_NDELAY);

        if (GPSUart_fd == -1)
            throw std::invalid_argument("[UART] GPS Uable to open the device:" + std::string(UartDevice));

        struct termios2 options;

        if (ioctl(GPSUart_fd, TCGETS2, &options) != 0)
        {
            close(GPSUart_fd);
            GPSUart_fd = -1;
        }
        options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
        options.c_iflag = 0;
        options.c_oflag = 0;
        options.c_lflag = 0;
        // if (0 != ioctl(GPSUart_fd, TCSETS2, &options))
        // {
        //     close(GPSUart_fd);
        //     GPSUart_fd = -1;
        // }
    }

private:
    int GPSUart_fd;
    std::string GPSDevice;
};