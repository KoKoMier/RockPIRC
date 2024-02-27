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
        CRSFUart_fd = open(UartDevice, O_RDWR | O_CLOEXEC | O_NONBLOCK);
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
            if (crc == hdr->frame.payload[hdr->frame.frameLength - 2])
            {
                switch (hdr->frame.type)
                {
                case crsfProtocol::CRSF_FRAMETYPE_GPS:
                    // packetGps(hdr);
                    return crsfProtocol::CRSF_FRAMETYPE_GPS;
                case crsfProtocol::CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
                    packetChannelsPacked(hdr, channelsOut);
                    return crsfProtocol::CRSF_FRAMETYPE_RC_CHANNELS_PACKED;
                case crsfProtocol::CRSF_FRAMETYPE_LINK_STATISTICS:
                    // packetLinkStatistics(hdr);
                    return crsfProtocol::CRSF_FRAMETYPE_LINK_STATISTICS;
                }
            }
        }
        return -1;
    }

    inline uint16_t rcToUs(uint16_t rc)
    {
        return (uint16_t)((rc * 0.62477120195241F) + 881);
    };

    inline ~CRSF()
    {
        close(CRSFUart_fd);
        delete dataBuffer;
    };

private:
    fd_set fd_Maker;
    uint8_t *dataBuffer;
    int CRSFUart_fd;
    int lose_frameCount;
    int InputBuffer;

    void packetChannelsPacked(const crsfProtocol::crsfFrame_t *p, int _channels[15])
    {
        crsfProtocol::crsfPayloadRcChannelsPacked_s *ch =
            (crsfProtocol::crsfPayloadRcChannelsPacked_s *)&p->frame.payload;
        _channels[0] = ch->chan0;
        _channels[1] = ch->chan1;
        _channels[2] = ch->chan2;
        _channels[3] = ch->chan3;
        _channels[4] = ch->chan4;
        _channels[5] = ch->chan5;
        _channels[6] = ch->chan6;
        _channels[7] = ch->chan7;
        _channels[8] = ch->chan8;
        _channels[9] = ch->chan9;
        _channels[10] = ch->chan10;
        _channels[11] = ch->chan11;
        _channels[12] = ch->chan12;
        _channels[13] = ch->chan13;
        _channels[14] = ch->chan14;
        _channels[15] = ch->chan15;
    }

    uint8_t gencrc(uint8_t *data, size_t len, uint8_t type)
    {
        size_t i, j;
        uint8_t crc = 0x00;
        // must check type at first, and skip
        crc ^= type;
        for (j = 0; j < 8; j++)
        {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ 0xd5);
            else
                crc <<= 1;
        }
        //
        for (i = 0; i < len; i++)
        {
            crc ^= data[i];
            for (j = 0; j < 8; j++)
            {
                if ((crc & 0x80) != 0)
                    crc = (uint8_t)((crc << 1) ^ 0xd5);
                else
                    crc <<= 1;
            }
        }
        return crc;
    }
};