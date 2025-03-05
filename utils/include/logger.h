#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include <mutex>
#include <syslog.h>

class Logger {
public:
    static Logger& instance();

    static void setup(bool bLogToStdout, int iMinLogLevel, const std::string& sIdent);

    static void log(int iPriority, const std::string& sMessage);

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool m_bLogToStdout;
    int m_iMinLogLevel;
    bool m_bIsSetup;
    std::mutex m_mutex;
};

#endif  // LOGGER_H_
