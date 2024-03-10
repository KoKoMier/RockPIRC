#pragma once
#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <iomanip>
#include <cmath>

class MTF02
{
public:
    inline MTF02(const char *i2cDevice, uint8_t i2caddr)
    {

        if ((fd = open(i2cDevice, O_RDWR)) < 0)
            throw -1;
        if (ioctl(fd, I2C_SLAVE, i2caddr) < 0)
            throw -2;
        if (ioctl(fd, I2C_TIMEOUT, 0x01) < 0) // set to 10ms?
            throw -2;

        char input = 0x00;
        ssize_t bytesWritten = write(fd, &input, 1);
        if (bytesWritten < 1)
        {
            std::cerr << "write error\n";
        }
        read(fd, &ID, 1);
        if (ID == 0x0F)
        {
            std::cout << "ID " << std::dec << (unsigned int)ID << "\r\n";
        }
    }
    inline void MTF02Read()
    {
        {
            uint8_t D[] = {0, 0, 0};
            char input = 0x01;
            ssize_t bytesWritten = write(fd, &input, 1);
            read(fd, &D, 3);
            int dinstance = (uint32_t)D[0] << 8 | (uint32_t)D[1];
            int qiangdu = D[2];
            // std::cout << "dinstance " << dinstance << "\r\n";
            // std::cout << "qiangdu " << (unsigned int)D[2] << "\r\n";
        }
        {
            uint8_t D[] = {0, 0, 0, 0, 0};
            char input = 0x06;
            ssize_t bytesWritten = write(fd, &input, 1);
            read(fd, &D, 5);
            int x = (uint32_t)D[0] << 8 | (uint32_t)D[1];
            int y = (uint32_t)D[2] << 8 | (uint32_t)D[3];
            int qiangdu = D[4];
            // std::cout << "x " << x << "\r\n";
            // std::cout << "y " << y << "\r\n";
            // std::cout << "qiangdu " << (unsigned int)D[4] << "\r\n";
        }
    }

private:
    uint8_t ID;
    int fd;
};