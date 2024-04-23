#include "alarmer.h"
#include "interceptor.h"
#include "exporter.h"

#include <chrono>

using namespace std;
using namespace qalarm;

Alarmer::Alarmer(vector<unique_ptr<MsgInterceptor>> interceptors, vector<unique_ptr<MsgExporter>> exporters)
    : m_table({{MsgLevel::FATAL, time_t(3)},
               {MsgLevel::ERROR, time_t(5)},
               {MsgLevel::WARN, time_t(10)},
               {MsgLevel::NOTICE, time_t(15)},
               {MsgLevel::INFO, time_t(60)},
               {MsgLevel::DEBUG, time_t(120)}})
    , m_interceptors(std::move(interceptors))
    , m_exporters(std::move(exporters))
    , m_queue(4096)
    , m_promise(promise<void>()) {
    m_thread = thread([this]() {
        auto fut = this->m_promise.get_future();
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
    });
}

Alarmer::~Alarmer() {
    m_promise.set_value();
    m_thread.join();
    for (auto &exp : m_exporters) {
        exp->Close();
    }
    for (auto &inter : m_interceptors) {
        inter->Close();
    }
}

int Alarmer::AlarmFatal(uint32_t code, string desc, map<string,string> annot) {
    auto ptr = make_unique<Message>(MsgLevel::FATAL, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        return -1;
    }
    return 0;
}

int Alarmer::AlarmError(uint32_t code, string desc, map<string,string> annot) {
    auto ptr = make_unique<Message>(MsgLevel::ERROR, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        return -1;
    }
    return 0;
}

int Alarmer::AlarmWarn(uint32_t code, string desc, map<string,string> annot) {
    auto ptr = make_unique<Message>(MsgLevel::WARN, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        return -1;
    }
    return 0;
}

int Alarmer::AlarmNotice(uint32_t code, string desc, map<string,string> annot) {
    auto ptr = make_unique<Message>(MsgLevel::NOTICE, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        return -1;
    }
    return 0;
}

int Alarmer::AlarmInfo(uint32_t code, string desc, map<string,string> annot) {
    auto ptr = make_unique<Message>(MsgLevel::INFO, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        return -1;
    }
    return 0;
}

int Alarmer::AlarmDebug(uint32_t code, string desc, map<string,string> annot) {
    auto ptr = make_unique<Message>(MsgLevel::DEBUG, code, std::move(desc), std::move(annot));
    auto ok = m_queue.TryPush(std::move(ptr));
    if (!ok) {
        return -1;
    }
    return 0;
}

bool Alarmer::check(unique_ptr<Message> &msg) {
    auto now = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
    auto id = msg->GeMsgId();
    auto it = m_limiter.find(id);
    if (it == m_limiter.end()) {
        m_limiter[id] = time_t(now);
        return true;
    }
    auto diff = now - it->second;
    if (diff < m_table[msg->GetMsgLevel()]) {
        return false;
    }
    it->second = now;
    return true;
}
