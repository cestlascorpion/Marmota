#include "alarm.h"
#include "interceptor.h"
#include "exporter.h"

using namespace std;
using namespace chrono;
using namespace qalarm;

namespace detail {
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
} // namespace detail

Alarm::Alarm(std::vector<std::unique_ptr<MsgInterceptor>> interceptors,
             std::vector<std::unique_ptr<MsgExporter>> exporters)
    : m_table({{MsgLevel::FATAL, MsgDurType(3)},
               {MsgLevel::ERROR, MsgDurType(5)},
               {MsgLevel::WARN, MsgDurType(10)},
               {MsgLevel::NOTICE, MsgDurType(15)},
               {MsgLevel::INFO, MsgDurType(60)},
               {MsgLevel::DEBUG, MsgDurType(120)}})
    , m_interceptors(std::move(interceptors))
    , m_exporters(std::move(exporters))
    , m_queue(4096)
    , m_promise(promise<void>()) {
    dzlog_debug("Alarm ctor ...");
    m_thread = thread([this]() {
        auto fut = this->m_promise.get_future();
        dzlog_debug("Alarm thread run");
        while (fut.wait_for(milliseconds(50)) != future_status::ready) {
            std::unique_ptr<Message> ptr{nullptr};
            if (this->m_queue.TryPop(ptr) && ptr != nullptr) {
                if (this->check(ptr)) {
                    for (const auto &inter : this->m_interceptors) {
                        inter->Intercept(ptr);
                    }
                    for (const auto &exp : this->m_exporters) {
                        exp->Export(ptr);
                    }
                }
            }
        }
        dzlog_debug("Alarm thread exit");
    });
    dzlog_debug("Alarm ctor done");
}

Alarm::~Alarm() {
    dzlog_debug("Alarm dctor...");
    m_promise.set_value();
    m_thread.join();
    dzlog_debug("Alarm thread join");
    for (auto &exp : m_exporters) {
        exp->Close();
    }
    for (auto &inter : m_interceptors) {
        inter->Close();
    }
    dzlog_debug("Alarm dctor done");
}

int Alarm::AlarmFatal(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::FATAL, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmFatal failed");
        return -1;
    }
    return 0;
}

int Alarm::AlarmError(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::ERROR, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmError failed");
        return -1;
    }
    return 0;
}

int Alarm::AlarmWarn(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::WARN, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmWarn failed");
        return -1;
    }
    return 0;
}

int Alarm::AlarmNotice(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::NOTICE, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmNotice failed");
        return -1;
    }
    return 0;
}

int Alarm::AlarmInfo(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::INFO, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmInfo failed");
        return -1;
    }
    return 0;
}

int Alarm::AlarmDebug(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::DEBUG, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmDebug failed");
        return -1;
    }
    return 0;
}

bool Alarm::check(unique_ptr<Message> &msg) {
    auto now = std::chrono::steady_clock::now();
    auto id = msg->GeMsgId();
    auto it = m_limiter.find(id);
    if (it == m_limiter.end()) {
        m_limiter[id] = MsgTpType(now);
        return true;
    }
    auto diff = now - it->second;
    if (diff < m_table[msg->GetMsgLevel()]) {
        // dzlog_debug("msg %lu ignore due to filter", id);
        return false;
    }
    it->second = now;
    return true;
}
