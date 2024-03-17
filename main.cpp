#include <fstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/time.h>
#include <csignal>
#include "src/QMC5883/QMC5883.hpp"
#include "src/M10QGPS/M10QGPS.hpp"
#include "src/CRSF/CRSFUart.hpp"
#include "src/MTF-02/MTF-02.hpp"

int TimestartUpLoad = 0;
int signalIn = 0;

inline int GetTimestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
}

int main(int argc, char *argv[])
{
    int argvs;
    TimestartUpLoad = GetTimestamp();

    while ((argvs = getopt(argc, argv, "h::s:S:g:G:a:t::")) != -1)
    {
        switch (argvs)
        {
        case 'h':
        {
            std::cout << "-g /dev/ttyS1 for GPS\n"
                      << "-G /dev/ttyS1 for GPS Parsed Data\n"
                      << "-S for QML5883 Compass Calibrate\n"
                      << "-s for QML5883 Compass\n"
                      << "-a /dev/ttyS2 for CRSF Uart\n"
                      << "-t for MTF-02\n";
        }
        break;
        case 't':
        {
            MTF02 *mtf02Test;
            try
            {
                mtf02Test = new MTF02("/dev/i2c-7", 0x31);
            }
            catch (int error)
            {
                switch (error)
                {
                case -1:
                    std::cout << "open I2C device faild\n";
                    break;
                case -2:
                    std::cout << "Set I2C device prop faild\n";
                    break;
                default:
                    std::cout << "Set I2C Command faild\n";
                    break;
                }
            }
            MTF02::MTF02Data Data;
            usleep(500);
            while (true)
            {
                Data = mtf02Test->MTF02DataGet();
                std::cout << " Speed_X " << Data.Speed_X << "\r\n";
                std::cout << " Speed_Y " << Data.Speed_Y << "\r\n";
                std::cout << " Speed_X " << Data.Pos_X << "\r\n";
                std::cout << " Speed_Y " << Data.Pos_Y << "\r\n";
                std::cout << " Distance " << Data.Distance << "\r\n";
                std::cout << "\033[5A";
                std::cout << "\033[K";
                usleep(5000);
            }
        }
        break;
        case 'a':
        {
            long int time;
            long int timee;
            CRSF test(optarg);
            int channelData[50];
            while (true)
            {
                time = GetTimestamp() - TimestartUpLoad;
                int retValue = test.CRSFRead(channelData);
                if (retValue > 0)
                {
                    for (size_t i = 0; i < 15; i++)
                    {
                        std::cout << test.rcToUs(channelData[i]) << " ";
                    }
                    std::cout << "\n";
                }
                else
                {
                    std::cout << "error frame recived"
                              << "\n";
                }

                timee = GetTimestamp() - TimestartUpLoad;
                std::cout << "ret: " << retValue
                          << " last frame time : " << timee - time << " "
                          << "\n";
            }
        }
        break;

        case 'S':
        {
            std::signal(SIGINT, [](int signal)
                        { signalIn = signal; });
            int rawx = 0;
            int rawy = 0;
            int rawz = 0;
            int calibration[10];
            calibration[0] = -5000;
            calibration[1] = 5000;
            calibration[2] = -5000;
            calibration[3] = 5000;
            calibration[4] = -5000;
            calibration[5] = 5000;

            QMC5883 mycompassTest(optarg, 0x0d);
            while (true)
            {
                mycompassTest.CompassGetRaw(rawx, rawy, rawz);
                std::cout << "x:" << std::setw(7) << std::setfill(' ') << rawx << ""
                          << "y:" << std::setw(7) << std::setfill(' ') << rawy << ""
                          << "z:" << std::setw(7) << std::setfill(' ') << rawz << "\r\n";
                mycompassTest.CompassCalibration(true, calibration);
                usleep(50 * 1000);
                if (signalIn == SIGINT)
                    break;
            }
            std::cout << "\r\n";
            std::cout << "XMAX: " << calibration[0] << "\n";
            std::cout << "XMIN: " << calibration[1] << "\n";
            std::cout << "YMAX: " << calibration[2] << "\n";
            std::cout << "YMIN: " << calibration[3] << "\n";
            std::cout << "ZMAX: " << calibration[4] << "\n";
            std::cout << "ZMIN: " << calibration[5] << "\n";
        }
        break;
        case 's':
        {
            int rawx = 0;
            int rawy = 0;
            int rawz = 0;
            double angle = 0;

            QMC5883 mycompassTest(optarg, 0x0d);
            mycompassTest.CompassApply(1010, -2408, 1347, -2110, 1695, -1755);
            while (true)
            {
                mycompassTest.CompassGetRaw(rawx, rawy, rawz);
                mycompassTest.CompassGetUnfixAngle(angle);
                std::cout << "Angle:" << std::setw(7) << std::setfill(' ') << angle << "\r\n";
                std::cout << "x:" << std::setw(7) << std::setfill(' ') << rawx << ""
                          << "y:" << std::setw(7) << std::setfill(' ') << rawy << ""
                          << "z:" << std::setw(7) << std::setfill(' ') << rawz << "\r\n";
                usleep(50 * 1000);
            }
        }
        break;
        case 'G':
        {
            long int time;
            long int timee;
            std::string GPSData;
            M10QGPS GPSUart(optarg);
            GPSUart.GPSReOpen();
            while (true)
            {
                if (GPSUart.GPSCheckDataAvaliable())
                {
                    time = GetTimestamp() - TimestartUpLoad;
                    std::cout << "\nlast frame time get:" << time - timee << "\r\n";
                    GPSUart.GPSRead(GPSData);
                    std::cout << GPSData;
                    timee = GetTimestamp() - TimestartUpLoad;
                }
                usleep(180000);
            }
        }
        break;
        case 'g':
        {
            long int time;
            long int timee;
            std::string GPSData;
            GPSUartData mydata;
            M10QGPS *GPSUart = new M10QGPS(optarg);
            GPSUart->GPSReOpen();
            while (true)
            {
                time = GetTimestamp() - TimestartUpLoad;
                mydata = GPSUart->GPSParse();
                std::cout << "satillites: " << mydata.satillitesCount << " ";
                std::cout << "DataError: " << mydata.DataUnCorrect << " ";
                std::cout << "lat: " << std::setprecision(9) << mydata.lat << " ";
                std::cout << "lng: " << std::setprecision(10) << mydata.lng << " \n";
                std::cout << "ALT: " << std::setprecision(4) << mydata.GPSAlititude << "M "
                          << "HDOP " << std::setprecision(4) << mydata.HDOP << " "
                          << "Quailty: " << mydata.GPSQuality << " "
                          << "GeoidalSP: " << mydata.GPSGeoidalSP << "\n";
                timee = GetTimestamp() - TimestartUpLoad;
                std::cout << "last frame time : " << timee - time << "\n";
                if ((timee - time) > 200000)
                    usleep(1500);
                else
                    usleep(200000 - (timee - time));
            }
        }
        break;
        }
    }
}
