#pragma once

#include <future>
#include <memory>
#include <vector>

#include "message.h"
#include "queue.h"

namespace qalarm {

class MsgInterceptor;
class MsgExporter;

class Alarmer {
public:
    Alarmer(std::vector<std::unique_ptr<MsgInterceptor>> interceptors,
            std::vector<std::unique_ptr<MsgExporter>> exporters);

    ~Alarmer();

public:
    // thread-safe
    int AlarmFatal(uint32_t code, std::string desc,
                   std::map<std::string, std::string> annot = std::map<std::string, std::string>());
    // thread-safe
    int AlarmError(uint32_t code, std::string desc,
                   std::map<std::string, std::string> annot = std::map<std::string, std::string>());
    // thread-safe
    int AlarmWarn(uint32_t code, std::string desc,
                  std::map<std::string, std::string> annot = std::map<std::string, std::string>());
    // thread-safe
    int AlarmNotice(uint32_t code, std::string desc,
                    std::map<std::string, std::string> annot = std::map<std::string, std::string>());
    // thread-safe
    int AlarmInfo(uint32_t code, std::string desc,
                  std::map<std::string, std::string> annot = std::map<std::string, std::string>());
    // thread-safe
    int AlarmDebug(uint32_t code, std::string desc,
                   std::map<std::string, std::string> annot = std::map<std::string, std::string>());

private:
    // thread-unsafe
    bool check(std::unique_ptr<Message> &msg);

private:
    std::map<MsgLevel, time_t> m_table;
    std::map<uint64_t, time_t> m_limiter;
    std::vector<std::unique_ptr<MsgInterceptor>> m_interceptors;
    std::vector<std::unique_ptr<MsgExporter>> m_exporters;
    MPSCQueue<std::unique_ptr<Message>> m_queue;
    std::promise<void> m_promise;
    std::thread m_thread;
};

} // namespace qalarm
