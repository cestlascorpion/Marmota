#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include <future>

#include "message.h"
#include "queue.h"

namespace qalarm {

class MsgInterceptor;
class MsgExporter;

class Alarm {
public:
    Alarm(std::vector<std::unique_ptr<MsgInterceptor>> interceptors,
          std::vector<std::unique_ptr<MsgExporter>> exporters);

    ~Alarm();

public:
    // thread-safe
    int AlarmFatal(uint32_t code, std::string desc, MsgKvType annot = MsgKvType());
    // thread-safe
    int AlarmError(uint32_t code, std::string desc, MsgKvType annot = MsgKvType());
    // thread-safe
    int AlarmWarn(uint32_t code, std::string desc, MsgKvType annot = MsgKvType());
    // thread-safe
    int AlarmNotice(uint32_t code, std::string desc, MsgKvType annot = MsgKvType());
    // thread-safe
    int AlarmInfo(uint32_t code, std::string desc, MsgKvType annot = MsgKvType());
    // thread-safe
    int AlarmDebug(uint32_t code, std::string desc, MsgKvType annot = MsgKvType());

private:
    bool check(std::unique_ptr<Message> &msg);

private:
    std::map<MsgLevel, MsgDurType> m_table;
    std::map<MsgIdType, MsgTpType> m_limiter;
    std::vector<std::unique_ptr<MsgInterceptor>> m_interceptors;
    std::vector<std::unique_ptr<MsgExporter>> m_exporters;
    MPSCQueue<std::unique_ptr<Message>> m_queue;
    std::promise<void> m_promise;
    std::thread m_thread;
};

} // namespace qalarm
