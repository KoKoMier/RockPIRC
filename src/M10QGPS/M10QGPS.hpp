#pragma once
#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <iomanip>
#include <cstring>
#include <asm-generic/ioctls.h>
#define termios asmtermios
#include <asm/termbits.h>
#undef termios
#include <termios.h>
#include <sys/select.h>
#include <string>

#define GGAData_LAT 2
#define GGAData_North 3
#define GGAData_LNG 4
#define GGAData_East 5
#define GGAData_Quality 6
#define GGAData_SATNUM 7
#define GGAData_HDOP 8
#define GGAData_Altitude 9
#define GGAData_GeoidalSP 11

struct GPSUartData
{
    double lat = 0;
    bool lat_North_Mode;
    double lng = 0;
    bool lat_East_Mode;

    int GPSQuality = 0;

    int satillitesCount = 0;
    double HDOP = 0;

    double GPSAlititude = 0;
    double GPSGeoidalSP = 0;

    bool DataUnCorrect;
};
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
        if (0 != ioctl(GPSUart_fd, TCSETS2, &options))
        {
            close(GPSUart_fd);
            GPSUart_fd = -1;
        }
        if (write(GPSUart_fd, GPSDisableGPGSVConfig, sizeof(GPSDisableGPGSVConfig)) == -1)
            throw std::invalid_argument("[UART] GPSWriteConfigError");
        else
        {
            tcdrain(GPSUart_fd);
            tcflush(GPSUart_fd, TCOFLUSH);
            if (write(GPSUart_fd, GPS5HzConfig, sizeof(GPS5HzConfig)) == -1)
                throw std::invalid_argument("[UART] GPSWriteConfigError");
            else
            {
                tcdrain(GPSUart_fd);
                tcflush(GPSUart_fd, TCOFLUSH);
                if (write(GPSUart_fd, Set_to_115kbps, sizeof(Set_to_115kbps)) == -1)
                    throw std::invalid_argument("[UART] GPSWriteConfigError");
                else
                {
                    tcdrain(GPSUart_fd);
                    tcflush(GPSUart_fd, TCOFLUSH);
                    close(GPSUart_fd);
                    GPSUart_fd = -1;
                }
            }
        }
    }

    inline void GPSReOpen()
    {
        if (GPSUart_fd != -1)
        {
            close(GPSUart_fd);
            GPSUart_fd = -1;
        }
        GPSUart_fd = open(GPSDevice.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
        if (GPSUart_fd == -1)
            std::invalid_argument("[UART] GPS115DeviceError");
        struct termios2 options_57;
        if (0 != ioctl(GPSUart_fd, TCGETS2, &options_57))
        {
            close(GPSUart_fd);
            GPSUart_fd = -1;
        }
        options_57.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
        options_57.c_iflag = IGNPAR;
        options_57.c_oflag = 0;
        options_57.c_lflag = 0;
        if (0 != ioctl(GPSUart_fd, TCSETS2, &options_57))
        {
            close(GPSUart_fd);
            GPSUart_fd = -1;
        }
        tcflush(GPSUart_fd, TCIOFLUSH);
        GPSPeeker = fdopen(GPSUart_fd, "r");
    }

    inline bool GPSCheckDataAvaliable()
    {
        int c[6];
        char GNRMC[6];
        int bytes_avaiable;
        ioctl(GPSUart_fd, FIONREAD, &bytes_avaiable);
        if (bytes_avaiable > 30)
        {
            for (size_t i = 0; i < 6; i++)
            {
                c[i] = fgetc(GPSPeeker);
                GNRMC[i] = (char)c[i];
            }
            for (size_t i = 0; i < 6; i++)
            {
                ungetc(c[i], GPSPeeker);
            }
            if (strncmp(GNRMC, "$GNRMC", 6) == 0 || strncmp(GNRMC, "CMRNG$", 6) == 0)
            {
                return true;
            }
            else
            {
                GPSReOpen();
                return false;
            }
        }
        else
        {
            return false;
        }
        return false;
    }

    inline int GPSRead(std::string &outputData)
    {
        outputData = "";
        if (GPSUart_fd == -1)
            return -1;
        FD_ZERO(&fd_Maker);
        FD_SET(GPSUart_fd, &fd_Maker);
        if (GPSCheckDataAvaliable())
        {
            int bytes_avaiable;
            ioctl(GPSUart_fd, FIONREAD, &bytes_avaiable);
            char TmpData[bytes_avaiable];
            int InputFrame;
            InputFrame = read(GPSUart_fd, TmpData, bytes_avaiable);
            if (InputFrame > 0)
            {
                for (size_t i = 0; i < InputFrame; i++)
                {
                    outputData += TmpData[i];
                }
            }
            tcflush(GPSUart_fd, TCIOFLUSH);
            return InputFrame;
        }
        return -1;
    }
    inline GPSUartData GPSParse()
    {
        int DataCount = 0;
        int GGADataCrash = 0;
        GPSUartData myData;
        myData.DataUnCorrect = false;
        std::string GPSDataStr;
        std::string GPSDataStrError;
        std::string GPSData[255];
        std::string GPSDataSub[255];
        std::string GPSTmpData[2];
        std::string GPSDataChecker[5];

        GPSRead(GPSDataStr);
        DataCount = dataParese(GPSDataStr, GPSData, "\r\n", 255);
        int Count = 0;
        while (GPSData[Count] != std::string(";"))
        {
            if (strncmp("$GNGGA", GPSData[Count].c_str(), 5) == 0)
            {
                GGADataCrash++;
                if (GGADataCrash > 1)
                    break;

                dataParese(GPSData[Count], GPSDataChecker, '*', 5);
                if (GPSDataChecker[1] == std::string(""))
                {
                    myData.DataUnCorrect = true;
                }

                dataParese(GPSData[Count], GPSDataSub, ',', 40);

                std::string GPSDataTmpLat = std::to_string(std::atof(GPSDataSub[GGAData_LAT].c_str()) / 100.0);
                dataParese(GPSDataTmpLat, GPSTmpData, '.', 2);
                myData.lat = std::atof(GPSTmpData[0].c_str()) * 10000.0;
                myData.lat += std::atof(GPSTmpData[1].c_str()) / 60.0;
                myData.lat = (int)(myData.lat * 100);

                std::string GPSDataTmpLng = std::to_string(std::atof(GPSDataSub[GGAData_LNG].c_str()) / 100.0);
                dataParese(GPSDataTmpLng, GPSTmpData, '.', 2);
                myData.lng = std::atof(GPSTmpData[0].c_str()) * 10000.0;
                myData.lng += std::atof(GPSTmpData[1].c_str()) / 60.0;
                myData.lng = (int)(myData.lng * 100);

                if (strncmp(GPSDataSub[GGAData_North].c_str(), "N", 1) == 0)
                    myData.lat_North_Mode = true;
                else
                    myData.lat_North_Mode = false;
                if (strncmp(GPSDataSub[GGAData_North].c_str(), "N", 1) == 0)
                    myData.lat_East_Mode = true;
                else
                    myData.lat_East_Mode = false;

                myData.GPSQuality = std::atof(GPSDataSub[GGAData_Quality].c_str());
                myData.satillitesCount = std::atof(GPSDataSub[GGAData_SATNUM].c_str());
                myData.HDOP = std::atof(GPSDataSub[GGAData_HDOP].c_str());
                myData.GPSAlititude = std::atof(GPSDataSub[GGAData_Altitude].c_str());
                myData.GPSGeoidalSP = std::atof(GPSDataSub[GGAData_GeoidalSP].c_str());
            }
            GPSLastDebug = GPSDataStr;
            Count++;
        }
        return myData;
    }

private:
    int GPSUart_fd;
    std::string GPSLastDebug;
    FILE *GPSPeeker;
    fd_set fd_Maker;
    std::string GPSDevice;
    uint8_t GPSDisableGPGSVConfig[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x03, 0x00, 0xFD, 0x15};
    uint8_t GPS5HzConfig[14] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A};
    uint8_t Set_to_115kbps[28] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x28, 0x00, 0x00,
                                  0x00, 0xC2, 0x01, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x3E};

    void dataParese(std::string data, std::string *databuff, const char split, int MaxSize)
    {
        std::istringstream f(data);
        std::string s;
        int count = 0;
        while (getline(f, s, split))
        {
            if (count > MaxSize)
                break;
            databuff[count] = s;
            count++;
        }
    }

    int dataParese(std::string data, std::string *databuff, std::string split, int MaxSize)
    {
        int Count = 0;
        size_t pos = 0;
        std::string token;
        while ((pos = data.find(split)) != std::string::npos)
        {
            if (Count > MaxSize)
                break;
            token = data.substr(0, pos);
            databuff[Count] = token;
            data.erase(0, pos + split.length());
            Count++;
        }
        databuff[Count] = data;
        databuff[Count + 1] = ";";

        return Count;
    }
};
