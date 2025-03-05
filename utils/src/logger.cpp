#include "logger.h"
#include <iostream>
#include <syslog.h>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_bLogToStdout(false),
      m_iMinLogLevel(LOG_DEBUG),
      m_bIsSetup(false)
{}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(m_mutex);
    closelog();
}

void Logger::setup(bool bLogToStdout, int iMinLogLevel, const std::string& sIdent) {
    std::lock_guard<std::mutex> lock(instance().m_mutex);

    if (!instance().m_bIsSetup) {
        instance().m_bLogToStdout = bLogToStdout;
        instance().m_iMinLogLevel = iMinLogLevel;
        // Open syslog with the provided identifier.
        openlog(sIdent.c_str(), LOG_PID | LOG_CONS, LOG_USER);
        instance().m_bIsSetup = true;
    }
}

void Logger::log(int iPriority, const std::string& sMessage) {
    std::lock_guard<std::mutex> lock(instance().m_mutex);

    if (!instance().m_bIsSetup) {
        return;
    }

    if (iPriority <= instance().m_iMinLogLevel) {
        syslog(iPriority, "%s", sMessage.c_str());
        if (instance().m_bLogToStdout) {
            std::cout << sMessage << std::endl;
        }
    }
}
