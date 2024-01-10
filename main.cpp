#include <fstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/time.h>
#include "src/QMC5883/QMC5883.hpp"
#include <csignal>

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

    while ((argvs = getopt(argc, argv, "s:S:g:m:")) != -1)
    {
        switch (argvs)
        {
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
            std::signal(SIGINT, [](int signal)
                        { signalIn = signal; });
            int rawx = 0;
            int rawy = 0;
            int rawz = 0;
            double angle = 0;

            QMC5883 mycompassTest(optarg, 0x0d);
            mycompassTest.CompassApply(2103, -1244, 1534, -689, 1834, -503);
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

        case 'g':
        {
            long int time;
            long int timee;
            while (true)
            {
                time = GetTimestamp() - TimestartUpLoad;

                timee = GetTimestamp() - TimestartUpLoad;
                if ((timee - time) > 200000)
                    usleep(1500);
                else
                    usleep(200000 - (timee - time));
            }
        }
        }
    }
}
