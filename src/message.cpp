#include "message.h"

#include <iomanip>
#include <time.h>

#include "json-c/json.h"

using namespace std;
using namespace qalarm;

namespace detail {

struct JsonGurad {
    JsonGurad()
        : job(json_object_new_object()) {
    }
    explicit JsonGurad(json_object *j)
        : job(j) {
    }

    ~JsonGurad() {
        json_object_put(job);
    }
    struct json_object *job;
};

string FormatMsgLevel(MsgLevel level) {
    switch (level) {
    case MsgLevel::FATAL:
        return "FATAL";
    case MsgLevel::ERROR:
        return "ERROR";
    case MsgLevel::WARN:
        return "WARN";
    case MsgLevel::NOTICE:
        return "NOTICE";
    case MsgLevel::INFO:
        return "INFO";
    case MsgLevel::DEBUG:
        return "DEBUG";
    }
    return "UNKNOWN";
}

string FormatTimePoint(chrono::time_point<std::chrono::system_clock> tp) {
    auto t = chrono::system_clock::to_time_t(tp);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return string(buffer);
}

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}; // namespace detail

Message::Message(MsgLevel level, uint32_t code, string desc, MsgKvType annot)
    : m_level(level)
    , m_code(code)
    , m_desc(std::move(desc))
    , m_annotation(std::move(annot))
    , m_tp(chrono::system_clock::now()) {
}

MsgIdType Message::GeMsgId() const {
    return std::hash<string>()(m_desc) ^ m_code;
}

MsgLevel Message::GetMsgLevel() const {
    return m_level;
}

void Message::SetAnnotation(const string &k, string v) {
    m_annotation[k] = std::move(v);
}

string Message::GetAnnotation(const string &k) const {
    auto it = m_annotation.find(k);
    if (it != m_annotation.end()) {
        return it->second;
    }
    return "";
}

std::string Message::ToString(unique_ptr<Message> &msg) {
    detail::JsonGurad json;
    json_object_object_add(json.job, "level", json_object_new_string(detail::FormatMsgLevel(msg->m_level).c_str()));
    json_object_object_add(json.job, "code", json_object_new_string(to_string(msg->m_code).c_str()));
    json_object_object_add(json.job, "desc", json_object_new_string(msg->m_desc.c_str()));
    json_object_object_add(json.job, "tp", json_object_new_string(detail::FormatTimePoint(msg->m_tp).c_str()));
    if (!msg->m_annotation.empty()) {
        auto annot = json_object_new_object();
        for (auto &it : msg->m_annotation) {
            json_object_object_add(annot, it.first.c_str(), json_object_new_string(it.second.c_str()));
        }
        json_object_object_add(json.job, "annot", annot);
    }
    return json_object_to_json_string(json.job);
}

std::unique_ptr<Message> Message::FromString(const string &str) {
    /*
    auto j = json_tokener_parse(str.c_str());
    if (!j) {
        return nullptr;
    }

    detail::JsonGurad json(j);
    auto level = json_object_object_get_ex(json.job, "level");
    if (!level) {
        return nullptr;
    }
    // TODO:
    auto code = json_object_object_get_ex(json.job, "code");
    if (!code) {
        return nullptr;
    }
    // TODO:
    auto desc = json_object_object_get_ex(json.job, "desc");
    if (!desc) {
        return nullptr;
    }
    // TODO:
    auto tp = json_object_object_get_ex(json.job, "tp");
    if (!tp) {
        return nullptr;
    }
    // TODO:

    auto msg = detail::make_unique<Message>(); // TODO:
    auto annot = json_object_object_get(json.job, "annot");
    if (annot != nullptr) {
        for (auto it = json_object_iterate(annot); it; it = json_object_iterate(annot)) {
            auto key = json_object_iter_key(it);
            auto val = json_object_iter_value(it);
            msg->m_annotation[key] = json_object_get_string(val);
        }
    }
    return msg;
    */
    return nullptr;
}
