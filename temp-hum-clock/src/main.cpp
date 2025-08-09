#include <atomic>
#include <iostream>
#include <cstdlib>
#include <getopt.h>
#include <pigpio.h>
#include <sstream>
#include <string>
#include <sys/syslog.h>
#include <unistd.h>
#include <csignal>
#include <iomanip>
#include <optional>
#include <thread>
#include <fstream>
#include <condition_variable>
#include <mutex>

#include "BoolReader.h"
#include "DHT11.h"
#include "TM1637.h"
#include "logger.h"

std::atomic_bool bTermSignal = false;
std::condition_variable cvTerminate;
std::mutex oMutex;
std::atomic<std::optional<float>> fTemp;
std::atomic<std::optional<float>> fHum;

const std::string DEFAULT_PIN_CONFIG = "/etc/temp-hum-clock";

class AppConfig {
public:
    bool m_bHumidity = false;
    bool m_bTemperature = false;
    bool m_bTime = false;
    bool m_bStdOut = false;
    int m_ilogLevel = LOG_INFO;
    time_t m_iShowDelay = 5; // Delay in seconds between changes
    std::string m_sPinConfigPath = DEFAULT_PIN_CONFIG;
};

class PinConfig {
public:

    int m_iDht11Pin = 17; // GPIO17 (BCM numbering, physical pin 11)
    int m_iDispIOPin = 23; // GPIO23 (BCM numbering, physical pin 16)
    int m_iDispClkPin = 18; // GPIO18 (BCM numbering, physical pin 12)
    int m_iLightSensorPin = 27; //GPIO27 (BCM numbering, physical pin 13)

    bool readPinConfig(std::string sFilepath) {

        std::ifstream oFile(sFilepath);
        std::string line;
        
        if (!oFile) {
            std::stringstream ss;
            ss << "Error opening file: " << sFilepath;
            return false;
        }
    
        while (std::getline(oFile, line)) {
            std::istringstream iss(line);
            std::string key;
            int value;
            
            if (std::getline(iss, key, '=') && (iss >> value)) {
                if (key == "TM1637_CLK") {
                    m_iDispClkPin = value;
                } else if (key == "TM1637_DIO") {
                    m_iDispIOPin = value;
                } else if (key == "DHT11_DATA") {
                    m_iDht11Pin = value;
                } else if (key == "LIGHT_SENSOR") {
                    m_iLightSensorPin = value;
                }
            }
        }

        return true;
    }
};

void signalHandler(int signal) {
    if (signal == SIGTERM || signal == SIGINT) {
        Logger::log(LOG_INFO, "Received signal: " + std::to_string(signal));
        bTermSignal.store(true);
        std::unique_lock <std::mutex> lock (oMutex);
        cvTerminate.notify_all();
    }
}

void printHelp(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -H, --humidity              Enable humidity display (default: false)\n"
              << "  -T, --temperature           Enable temperature display (default: false)\n"
              << "  -t, --time                  Enable time display (default: false)\n"
              << "  -c  --pin-config            Configuration file (default: " << DEFAULT_PIN_CONFIG << ")\n"
              << "  -d, --delay <seconds>       Set delay in seconds between changes (default: 10)\n"
              << "  -s, --stdout                Output logs to stdout (default: false)\n"
              << "  -p, --loglevel              Set the log level (default: 6 - LOG_INFO)\n"
              << "  -h, --help                  Show this help message\n";
}

bool parseCommandLineArguments(int argc, char* argv[], AppConfig &config) {
    bool debugEnabled = false;

    // Define the long options.
    static struct option long_options[] = {
        {"humidity",    no_argument,       0, 'H'},
        {"temperature", no_argument,       0, 'T'},
        {"time",        no_argument,       0, 't'},
        {"delay",       required_argument, 0, 'd'},
        {"stdout",      no_argument,       0, 's'},
        {"loglevel",    required_argument, 0, 'p'},
        {"help",        no_argument,       0, 'h'},
        {"pin-config",  required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    // Option string: 'd' requires an argument (hence the colon).
    const char* optionString = "HTtd:sp:hc:";

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, optionString, long_options, &option_index)) != -1) {
        switch(c) {
            case 'H': // --humidity
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
                break;

            case 'c': // --config
                config.m_sPinConfigPath = optarg;
                break;

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

void dht11Runner(const PinConfig& oConf) {
    addons::DHT11 oDht11(oConf.m_iDht11Pin);

    while(!bTermSignal.load()) {
        float fTmpTemp;
        float fTmpHum;

        if(oDht11.read( fTmpTemp, fTmpHum)) {
            fHum.store(fTmpHum);
            fTemp.store(fTmpTemp);
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << fTemp.load().value() << "C*\t" << fHum.load().value();
            Logger::log(LOG_DEBUG, "dht11Runner| Getting data from the sensor:" + ss.str());
        } else {
            Logger::log(LOG_ERR, "Failed to get info from the DHT11 sensor");
        }

        std::unique_lock <std::mutex> lock (oMutex);
        auto sec = std::chrono::seconds(20);
        cvTerminate.wait_for(lock, sec);
    }
}

void updateDisplayDuringTime(const AppConfig &oConf, addons::TM1637 &oTM1637) {
    if (oConf.m_bTime) {
        auto endTime = std::chrono::system_clock::now() + std::chrono::seconds(oConf.m_iShowDelay);
        while (std::chrono::system_clock::now() < endTime && !bTermSignal.load()) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm* local_tm = std::localtime(&now_c);

            std::ostringstream oss;
            oss << std::setw(2) << std::setfill('0') << local_tm->tm_hour
                << std::setw(2) << std::setfill('0') << local_tm->tm_min;
            std::string timeStr = oss.str();

            oTM1637.display(timeStr, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void TM1637Runner(const AppConfig& oConf, const PinConfig& oPinConf) {
    addons::TM1637 oTM1637(oPinConf.m_iDispIOPin, oPinConf.m_iDispClkPin);
    addons::BoolReader oLightSensor(oPinConf.m_iLightSensorPin);

    bool bLight = false;
    if (!oLightSensor.read(bLight)) {
        // If it fails to get the brightness, make the brightness max
        bLight = true;
    }
    oTM1637.setBrightness(bLight ? 6 : 2);


    oTM1637.display("Run", false);

    while(!bTermSignal.load()) {
        float fTmpTemp;
        float fTmpHum;

        std::stringstream ss;

        if (oConf.m_bTime) {
            updateDisplayDuringTime(oConf, oTM1637);
        }

        if (bTermSignal.load()) {
            break;
        }

        if (oConf.m_bTemperature && fTemp.load().has_value()) {
            ss.clear();
            float fTmpTemp = fTemp.load().value();
            if (fTmpTemp < 0) {
                ss << std::setw(3) << std::fixed << std::setprecision(0) << fTmpTemp << "*";
            } else {
                ss << std::setw(2) << std::fixed << std::setprecision(0) << fTmpTemp << "*C";
            }

            oTM1637.display(ss.str(), false);

            std::unique_lock <std::mutex> lock (oMutex);
            auto sec = std::chrono::seconds(oConf.m_iShowDelay);
            cvTerminate.wait_for(lock, sec);
        }

        if (bTermSignal.load()) {
            break;
        }

        if (oConf.m_bHumidity && fHum.load().has_value()) {
            ss.clear();
            float fTmpHum = fHum.load().value();
            ss << std::setw(4) << std::setprecision(0) << fTmpHum;

            oTM1637.display(ss.str(), false);

            std::unique_lock <std::mutex> lock (oMutex);
            auto sec = std::chrono::seconds(oConf.m_iShowDelay);
            cvTerminate.wait_for(lock, sec);
        }
    }

    oTM1637.display("    ", false);
    oTM1637.setBrightness(0);
}

int main(int argc, char* argv[]) {
    if (gpioInitialise() < 0) {
        Logger::log(LOG_ERR, "Failed to initialize GPIO");
        return 1;
    }

    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);

    AppConfig config;
    if (!parseCommandLineArguments(argc, argv, config)) {
        return 1;
    }

    Logger::setup(config.m_bStdOut, config.m_ilogLevel, "temp-hum-clock");

    if (!config.m_bTime && !config.m_bTemperature && !config.m_bHumidity) {
        Logger::log(LOG_WARNING, "All options to display are disabled. Exiting");
        return 0;
    }

    PinConfig pinConfig;
    pinConfig.readPinConfig(config.m_sPinConfigPath);

    std::thread DHT11Thread(dht11Runner, pinConfig);
    std::thread TM1637Thread(TM1637Runner, config, pinConfig);

    DHT11Thread.join();
    TM1637Thread.join();

    gpioTerminate();

    Logger::log(LOG_INFO, "Graceful terminating... ");
    return 0;
}
