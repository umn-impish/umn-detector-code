#include <logging.hh>
#include <mutex>

#include <syslog.h>
#include <systemd/sd-journal.h>

// log functions need to be thread safe
static std::mutex mux;

void log_info(const std::string& s) {
    std::lock_guard<std::mutex> l(mux);
    sd_journal_print(LOG_INFO, "%s", s.c_str());
}

void log_debug(const std::string& s) {
    std::lock_guard<std::mutex> l(mux);
    sd_journal_print(LOG_DEBUG, "%s", s.c_str());
}

void log_warning(const std::string& s) {
    std::lock_guard<std::mutex> l(mux);
    sd_journal_print(LOG_WARNING, "%s", s.c_str());
}

void log_error(const std::string& s) {
    std::lock_guard<std::mutex> l(mux);
    sd_journal_print(LOG_ERR, "%s", s.c_str());
}
