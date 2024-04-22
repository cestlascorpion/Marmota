#include "alarmer.h"
#include "interceptor.h"
#include "exporter.h"

#include <chrono>

using namespace std;
using namespace qalarm;

namespace detail {

// make_unique support for pre c++14
#if __cplusplus >= 201402L // C++14 and beyond
using std::make_unique;
#else
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
    static_assert(!std::is_array<T>::value, "arrays not supported");
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif
} // namespace detail

Alarmer::Alarmer(vector<unique_ptr<MsgInterceptor>> interceptors, vector<unique_ptr<MsgExporter>> exporters)
    : m_table({{MsgLevel::FATAL, MsgTpType(3)},
               {MsgLevel::ERROR, MsgTpType(5)},
               {MsgLevel::WARN, MsgTpType(10)},
               {MsgLevel::NOTICE, MsgTpType(15)},
               {MsgLevel::INFO, MsgTpType(60)},
               {MsgLevel::DEBUG, MsgTpType(120)}})
    , m_interceptors(std::move(interceptors))
    , m_exporters(std::move(exporters))
    , m_queue(4096)
    , m_promise(promise<void>()) {
    dzlog_debug("Alarmer ctor ...");
    m_thread = thread([this]() {
        auto fut = this->m_promise.get_future();
        dzlog_debug("Alarmer thread run");

        while (fut.wait_for(chrono::milliseconds(10)) != future_status::ready) {
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
        dzlog_debug("Alarmer thread exit");
    });
    dzlog_debug("Alarmer ctor done");
}

Alarmer::~Alarmer() {
    dzlog_debug("Alarmer dctor...");
    m_promise.set_value();
    m_thread.join();
    dzlog_debug("Alarmer thread join");
    for (auto &exp : m_exporters) {
        exp->Close();
    }
    for (auto &inter : m_interceptors) {
        inter->Close();
    }
    dzlog_debug("Alarmer dctor done");
}

int Alarmer::AlarmFatal(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::FATAL, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmerFatal failed");
        return -1;
    }
    return 0;
}

int Alarmer::AlarmError(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::ERROR, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmerError failed");
        return -1;
    }
    return 0;
}

int Alarmer::AlarmWarn(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::WARN, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmerWarn failed");
        return -1;
    }
    return 0;
}

int Alarmer::AlarmNotice(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::NOTICE, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmerNotice failed");
        return -1;
    }
    return 0;
}

int Alarmer::AlarmInfo(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::INFO, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmerInfo failed");
        return -1;
    }
    return 0;
}

int Alarmer::AlarmDebug(uint32_t code, string desc, MsgKvType annot) {
    auto ptr = detail::make_unique<Message>(MsgLevel::DEBUG, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        dzlog_debug("AlarmerDebug failed");
        return -1;
    }
    return 0;
}

bool Alarmer::check(unique_ptr<Message> &msg) {
    auto now = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
    auto id = msg->GeMsgId();
    auto it = m_limiter.find(id);
    if (it == m_limiter.end()) {
        m_limiter[id] = MsgTpType(now);
        return true;
    }
    auto diff = now - it->second;
    if (diff < m_table[msg->GetMsgLevel()]) {
        dzlog_debug("msg %lu ignore due to filter", id);
        return false;
    }
    it->second = now;
    return true;
}
