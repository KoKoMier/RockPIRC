#pragma once
#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <iomanip>
#include <cmath>
#define PI 3.1415

#define QMC5883_REG_PRODUCTID 0x0D

#define QMC5883_REG_HRESET 0x0b
#define QMC5883_OP_HRESET 0x01

#define QMC5883_REG_CTRL2 0x0a
#define QMC5883_CMD_SOFT_RST_ENABLE 0x80

#define QMC5883_REG_CTRL1 0x09
#define QMC5883_CMD_MODE_CON 0x01
#define QMC5883_CMD_ODR_200HZ 0x0C
#define QMC5883_CMD_RNG_8G 0x10
#define QMC5883_CMD_OSR_512 0x00

#define QMC5883_REG_DATA 0x00
class QMC5883
{
public:
    QMC5883(const char *i2cDevice, uint8_t i2caddr)
    {
        QMC5883FD = open(i2cDevice, O_RDWR);
        if (ioctl(QMC5883FD, I2C_SLAVE, i2caddr) < 0)
            std::invalid_argument("[I2C] QMC5883 Unable to open device" + std::string(i2cDevice));

        usleep(1000);
        if (ioctl(QMC5883FD, I2C_TIMEOUT, 0x01) < 0)
            throw -1;
        usleep(5000);

        {
            uint8_t wdata[2] = {QMC5883_REG_HRESET, QMC5883_OP_HRESET};
            if (write(QMC5883FD, &wdata, 2) < 0)
                throw std::invalid_argument("[i2c] init QMC5883 error");
        }
        usleep(1000);
        {
            uint8_t wdata[1] = {QMC5883_REG_PRODUCTID};
            uint8_t rdata[1] = {0};
            write(QMC5883FD, &wdata, 1);
            read(QMC5883FD, &rdata, 1);
            if (static_cast<int>(rdata[0]) != 255)
                throw std::invalid_argument("[i2c] init QMC5883 error");
        }
        {
            uint8_t wdata[2] = {QMC5883_REG_CTRL1, QMC5883_CMD_MODE_CON | QMC5883_CMD_ODR_200HZ | QMC5883_CMD_RNG_8G | QMC5883_CMD_OSR_512};
            if (write(QMC5883FD, &wdata, 2) < 0)
                throw std::invalid_argument("[I2C] init compass error");
        }

        CompassCalibrationData[0] = 0;
        CompassCalibrationData[1] = 0;
        CompassCalibrationData[2] = 0;
        CompassCalibrationData[3] = 0;
        CompassCalibrationData[4] = 0;
        CompassCalibrationData[5] = 0;
        CompassCalibrationData[6] = 0;
        CompassCalibrationData[7] = 1;
        CompassCalibrationData[8] = 1;
        CompassCalibrationData[9] = 1;
    }

    int CompassGetRaw(int &RawMAGX, int &RawMAGY, int &RawMAGZ)
    {
        int error = CompassRead(RawMAGIX, RawMAGIY, RawMAGIZ);
        int tRawMAGX = ((double)RawMAGIX - CompassCalibrationData[0]) * CompassCalibrationData[7];
        int tRawMAGY = ((double)RawMAGIY - CompassCalibrationData[1]) * CompassCalibrationData[8];
        int tRawMAGZ = ((double)RawMAGIZ - CompassCalibrationData[2]) * CompassCalibrationData[9];
        RawMAGCX = tRawMAGX;
        RawMAGCY = tRawMAGY;
        RawMAGCZ = tRawMAGZ;
        RawMAGX = RawMAGCX;
        RawMAGY = RawMAGCY;
        RawMAGZ = RawMAGCZ;
        return error;
    }

    void CompassCalibration(bool Calibrating, int *CalibratedData)
    {
        CompassCalibrationData[0] = 0;
        CompassCalibrationData[1] = 0;
        CompassCalibrationData[2] = 0;
        CompassCalibrationData[7] = 1;
        CompassCalibrationData[8] = 1;
        CompassCalibrationData[9] = 1;

        if (Calibrating)
        {
            CalibratedData[0] = RawMAGCX > CalibratedData[0] ? RawMAGCX : CalibratedData[0];
            CalibratedData[1] = RawMAGCX < CalibratedData[1] ? RawMAGCX : CalibratedData[1];
            CalibratedData[2] = RawMAGCY > CalibratedData[2] ? RawMAGCX : CalibratedData[2];
            CalibratedData[3] = RawMAGCY < CalibratedData[3] ? RawMAGCX : CalibratedData[3];
            CalibratedData[4] = RawMAGCZ > CalibratedData[4] ? RawMAGCX : CalibratedData[4];
            CalibratedData[5] = RawMAGCZ < CalibratedData[6] ? RawMAGCX : CalibratedData[5];
        }
        else
        {
            CalibratedData[0] = RawMAGCX;
            CalibratedData[1] = RawMAGCX;
            CalibratedData[2] = RawMAGCY;
            CalibratedData[3] = RawMAGCY;
            CalibratedData[4] = RawMAGCZ;
            CalibratedData[5] = RawMAGCZ;
        }
    }

    void CompassApply(int XMAX, int XMIN, int YMAX, int YMIN, int ZMAX, int ZMIN)
    {
        CompassCalibrationData[0] = (XMAX + XMIN) / 2;
        CompassCalibrationData[1] = (YMAX + YMIN) / 2;
        CompassCalibrationData[2] = (ZMAX + ZMIN) / 2;
        CompassCalibrationData[3] = (YMAX - YMIN) / 2;
        CompassCalibrationData[4] = (YMAX - YMIN) / 2;
        CompassCalibrationData[5] = (YMAX - YMIN) / 2;
        CompassCalibrationData[6] = (CompassCalibrationData[3] + CompassCalibrationData[4] + CompassCalibrationData[5]) / 3;
        CompassCalibrationData[7] = CompassCalibrationData[6] / CompassCalibrationData[3];
        CompassCalibrationData[8] = CompassCalibrationData[6] / CompassCalibrationData[4];
        CompassCalibrationData[9] = CompassCalibrationData[6] / CompassCalibrationData[5];
    }

    void CompassGetUnfixAngle(double &UnFixAngle)
    {
        UnFixAngle = atan2((float)RawMAGCY, (float)-1 * RawMAGCX) * 180.f / PI;
        if (UnFixAngle < 0)
            UnFixAngle += 360;
        else if (UnFixAngle > 0)
            UnFixAngle -= 360;
    }

private:
    int CompassRead(int &RawMAGX, int &RawMAGY, int &RawMAGZ)
    {
        int error = -1;
        {
            uint8_t wdata[1] = {QMC5883_REG_DATA};
            uint8_t rdata[6] = {0};
            write(QMC5883FD, &wdata, 1);
            error = read(QMC5883FD, &rdata, 6);
            if (error = 6)
            {
                int Tmp_MX = (short)(rdata[1] << 8 | rdata[0]);
                int Tmp_MY = (short)(rdata[3] << 8 | rdata[2]);
                int Tmp_MZ = (short)(rdata[5] << 8 | rdata[4]);
                RawMAGX = Tmp_MX;
                RawMAGY = Tmp_MY;
                RawMAGZ = Tmp_MZ;
            }
        }
        return error;
    }

    int RawMAGIX = 0;
    int RawMAGIY = 0;
    int RawMAGIZ = 0;

    int RawMAGCX = 0;
    int RawMAGCY = 0;
    int RawMAGCZ = 0;

    int QMC5883FD;
    double CompassCalibrationData[10] = {0};
};