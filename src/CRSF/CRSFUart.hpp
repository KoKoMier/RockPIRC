#pragma once

#include "CRSFProtocol.hpp"
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <asm/termbits.h>
#include <sys/ioctl.h>
#include <asm-generic/ioctls.h>
#include <sys/select.h>
#include <iostream>

#define CRSF_MAX_READ_SIZE 500
#define CRSF_DEFAULT_BANDRATE 420000

namespace CRSFTelemetry
{

};

class CRSF
{
public:
    inline CRSF(const char *UartDevice, int bandrate = CRSF_DEFAULT_BANDRATE)
    {
        dataBuffer = new uint8_t[CRSF_MAX_READ_SIZE];
        CRSFUart_fd = open("/dev/ttyS1", O_RDWR | O_CLOEXEC | O_NONBLOCK);
        if (CRSFUart_fd == -1)
            throw std::invalid_argument("[UART] CRSF Uable to open device:" + std::string(UartDevice));
        struct termios2 options;

        if (0 != ioctl(CRSFUart_fd, TCGETS2, &options))
        {
            close(CRSFUart_fd);
            CRSFUart_fd = -1;
        }
        options.c_cflag = B460800 | CS8 | CLOCAL | CREAD;
        options.c_cflag &= ~CBAUD;
        options.c_cflag |= BOTHER;
        options.c_iflag = 0;
        options.c_oflag = 0;
        options.c_lflag = 0;
        options.c_ispeed = bandrate;
        options.c_ospeed = bandrate;

        if (0 != ioctl(CRSFUart_fd, TCSETS2, &options))
        {
            close(CRSFUart_fd);
            CRSFUart_fd = -1;
            throw std::invalid_argument("[UART] CRSF init failed");
        }
    }

    inline int CRSFRead(int *channelsData, int timeout = 100000)
    {
        int ret = -1;
        if (CRSFUart_fd == -1)
            return -1;

        //
        FD_ZERO(&fd_Maker);
        FD_SET(CRSFUart_fd, &fd_Maker);
        lose_frameCount = 0;
        //
        timeval timecl;
        timecl.tv_sec = 0;
        timecl.tv_usec = timeout;
        int err = select(CRSFUart_fd + 1, &fd_Maker, NULL, NULL, &timecl);
        //
        InputBuffer = read(CRSFUart_fd, dataBuffer, CRSF_MAX_READ_SIZE - 2);
        std::cout << "InputBuffer: " << InputBuffer
                  << "\r\n";

        if (InputBuffer > 3)
        {
            ret = CRSFParser(dataBuffer, InputBuffer, channelsData);
        }
        return ret;
    }
    inline int CRSFParser(uint8_t *data, int size, int channelsOut[15])
    {
        const crsfProtocol::crsfFrame_t *hdr = (crsfProtocol::crsfFrame_t *)data;
        if (hdr->frame.deviceAddress == crsfProtocol::CRSF_ADDRESS_FLIGHT_CONTROLLER)
        {
            uint8_t crc = gencrc((uint8_t *)(hdr->frame.payload), hdr->frame.frameLength - 2, hdr->frame.type);
            std::cout << "type: " << hdr->frame.type << "\r\n";
            if (crc == hdr->frame.payload[hdr->frame.frameLength - 2])
            {
            }
        }
    }

private:
    fd_set fd_Maker;
    uint8_t *dataBuffer;
    int CRSFUart_fd;
    int lose_frameCount;
    int InputBuffer;

    uint8_t gencrc(uint8_t *data, size_t len, uint8_t type)
    {
        size_t i, j;
        uint8_t crc = 0x00;
    }
};