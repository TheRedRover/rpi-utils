#include <iostream>
#include <cstdlib>
#include <getopt.h>
#include <pigpio.h>
#include <sstream>
#include <string>
#include <sys/syslog.h>
#include <iomanip>
#include <unistd.h>
#include <csignal>

#include "DHT11.h"
#include "logger.h"

bool bTermSignal = false;

class AppConfig {
    public:
        int m_iPin = 17; // GPIO17 (BCM numbering, physical pin 11)
        bool m_bHumidity = false;
        bool m_bTemperature = false;
        bool m_bTime = false;
        bool m_bStdOut = false;
        int m_ilogLevel = LOG_DEBUG;
        time_t m_iShowDelay = 10; // Delay in seconds between changes
};

void signalHandler(int signal) {
    if (signal == SIGTERM || signal == SIGINT) {
        Logger::log(LOG_INFO, "Received signal: " + std::to_string(signal));
        bTermSignal = true;
    }
}

void printHelp(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -U, --humidity              Enable humidity display (default: false)\n"
              << "  -T, --temperature           Enable temperature display (default: false)\n"
              << "  -t, --time                  Enable time display (default: false)\n"
              << "  -d, --delay <seconds>       Set delay in seconds between changes (default: 10)\n"
              << "  -s, --stdout                Output logs to stdout (default: false)\n"
              << "  -p, --loglevel              Set the log level (default: 7 - LOG_DEBUG)\n"
              << "  -h, --help                  Show this help message\n";
}

bool parseCommandLineArguments(int argc, char* argv[], AppConfig &config) {
    bool debugEnabled = false;

    // Define the long options.
    static struct option long_options[] = {
        {"humidity",    no_argument,       0, 'U'},
        {"temperature", no_argument,       0, 'T'},
        {"time",        no_argument,       0, 't'},
        {"delay",       required_argument, 0, 'd'},
        {"stdout",      no_argument,       0, 's'},
        {"loglevel",    required_argument, 0, 'p'},
        {"help",        no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    // Option string: 'd' requires an argument (hence the colon).
    const char* optionString = "UTtd:gsh";

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, optionString, long_options, &option_index)) != -1) {
        switch(c) {
            case 'U': // --humidity
                config.m_bHumidity = true;
                break;

            case 'T': // --temperature
                config.m_bTemperature = true;
                break;

            case 't': // --time
                config.m_bTime = true;
                break;

            case 'd': // --delay
                try {
                    config.m_iShowDelay = std::stoi(optarg);
                } catch (const std::invalid_argument & e) {
                    std::cerr << "Parsing error: invalid argument for show delay ["
                    << optarg
                    << "]: ["
                    << e.what()
                    << "]" << std::endl;
                    return false;
                } catch (const std::out_of_range & e) {
                    std::cerr << "Parsing error: out of range for show delay ["
                    << optarg
                    << "]: ["
                    << e.what()
                    << "]" << std::endl;
                    return false;
                }

                break;

            case 's': // --stdout
                config.m_bStdOut = true;
                break;

            case 'p': // --loglevel
                try {
                    config.m_ilogLevel = std::stoi(optarg);
                } catch (const std::invalid_argument & e) {
                    std::cerr << "Parsing error: invalid argument for log level ["
                    << optarg
                    << "]: ["
                    << e.what()
                    << "]" << std::endl;
                    return false;
                } catch (const std::out_of_range & e) {
                    std::cerr << "Parsing error: out of range for log level ["
                    << optarg
                    << "]: ["
                    << e.what()
                    << "]" << std::endl;
                    return false;
                }

            case 'h': // --help
                printHelp(argv[0]);
                exit(0);

            case '?': // Unknown option encountered
                std::cerr << "Error: Unknown option encountered.\n";
                printHelp(argv[0]);
                return false;

            default:
                break;
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (gpioInitialise() < 0) {
        Logger::log(LOG_ERR, "HDT11| Failed to initialize GPIO");
        return 1;
    }

    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);

    AppConfig config;
    if (!parseCommandLineArguments(argc, argv, config)) {
        return 1;
    }

    Logger::setup(config.m_bStdOut, config.m_ilogLevel, "temp-hum-clock");

    addons::DHT11 oDht11;
    oDht11.attach(config.m_iPin);

    float fTemp = 0;
    float fHum = 0;

    while (!bTermSignal) {
        if (!oDht11.read(fTemp, fHum)) {
            Logger::log(LOG_ERR, "Unable to read data from the sensor");
        }
        else {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << fTemp << "C, "  << fHum;
            Logger::log(LOG_INFO, ss.str());
        }
        sleep(20);
    }

    gpioTerminate();

    Logger::log(LOG_INFO, "Graceful terminating... ");
    return 0;
}
