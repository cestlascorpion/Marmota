#include "message.h"

#include <chrono>
#include <cstring>
#include <ctime>

#include "json-c/json.h"

using namespace std;
using namespace qalarm;

namespace detail {

struct JsonGuard {
    explicit JsonGuard(json_object *obj)
        : job(obj) {}
    ~JsonGuard() {
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

MsgLevel ParseMsgLevel(const char *level) {
    if (level == nullptr || strlen(level) < 1) {
        return MsgLevel::DEBUG;
    }
    switch (level[0]) {
    case 'F':
        return MsgLevel::FATAL;
    case 'E':
        return MsgLevel::ERROR;
    case 'W':
        return MsgLevel::WARN;
    case 'N':
        return MsgLevel::NOTICE;
    case 'I':
        return MsgLevel::INFO;
    case 'D':
        return MsgLevel::DEBUG;
    }
    return MsgLevel::DEBUG;
}

string FormatMsgCode(uint32_t code) {
    char c[11]{0};
    sprintf(c, "0x%08X", code);
    return string{c};
}

uint32_t ParseMsgCode(const char *code) {
    if (code == nullptr || strlen(code) < 3 || code[0] != '0' || code[1] != 'x') {
        return 0;
    }
    return static_cast<uint32_t>(strtoul(code + 2, nullptr, 16));
}

string FormatTimePoint(time_t tp) {
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&tp));
    return string{buffer};
}

time_t ParseTimePoint(const char *tp) {
    if (tp == nullptr) {
        return 0;
    }
    struct tm tm {};
    memset(&tm, 0, sizeof(tm));
    if (strptime(tp, "%Y-%m-%d %H:%M:%S", &tm) == nullptr) {
        return 0;
    }
    return mktime(&tm);
}

} // namespace detail

Message::Message(MsgLevel level, uint32_t code, string desc, map<string, string> annot)
    : m_lv(level)
    , m_co(code)
    , m_tp(chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count())
    , m_desc(std::move(desc))
    , m_annotation(std::move(annot)) {}

Message::Message(MsgLevel level, uint32_t code, string desc, time_t tp, map<string, string> annot)
    : m_lv(level)
    , m_co(code)
    , m_tp(tp)
    , m_desc(std::move(desc))
    , m_annotation(std::move(annot)) {}

uint64_t Message::GeMsgId() const {
    return std::hash<string>()(m_desc) ^ m_co;
}

MsgLevel Message::GetMsgLevel() const {
    return m_lv;
}

void Message::SetAnnotation(const string &k, string v) {
    m_annotation[k] = std::move(v);
}

std::string Message::ToString(unique_ptr<Message> &msg) {
    detail::JsonGuard json(json_object_new_object());
    auto lv = detail::FormatMsgLevel(msg->m_lv);
    json_object_object_add(json.job, "level", json_object_new_string(lv.c_str()));
    auto co = detail::FormatMsgCode(msg->m_co);
    json_object_object_add(json.job, "code", json_object_new_string(co.c_str()));
    auto tp = detail::FormatTimePoint(msg->m_tp);
    json_object_object_add(json.job, "tp", json_object_new_string(tp.c_str()));
    auto desc = msg->m_desc;
    json_object_object_add(json.job, "desc", json_object_new_string(desc.c_str()));
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
    auto obj = json_tokener_parse(str.c_str());
    if (!obj) {
        return nullptr;
    }

    detail::JsonGuard json(obj);
    struct json_object *tmp = nullptr;
    if (json_object_object_get_ex(json.job, "level", &tmp) != 1) {
        return nullptr;
    }
    if (json_object_get_type(tmp) != json_type_string) {
        return nullptr;
    }
    MsgLevel level = detail::ParseMsgLevel(json_object_get_string(tmp));

    if (json_object_object_get_ex(json.job, "code", &tmp) != 1) {
        return nullptr;
    }
    if (json_object_get_type(tmp) != json_type_string) {
        return nullptr;
    }
    uint32_t code = detail::ParseMsgCode(json_object_get_string(tmp));

    if (json_object_object_get_ex(json.job, "tp", &tmp) != 1) {
        return nullptr;
    }
    if (json_object_get_type(tmp) != json_type_string) {
        return nullptr;
    }
    time_t tp = detail::ParseTimePoint(json_object_get_string(tmp));

    if (json_object_object_get_ex(json.job, "desc", &tmp) != 1) {
        return nullptr;
    }
    if (json_object_get_type(tmp) != json_type_string) {
        return nullptr;
    }
    string desc = json_object_get_string(tmp);

    auto msg = make_unique<Message>(level, code, desc, tp);
    if (json_object_object_get_ex(json.job, "annot", &tmp) == 1 && json_object_get_type(tmp) == json_type_object) {
        json_object_object_foreach(tmp, key, val) {
            if (json_object_get_type(val) == json_type_string) {
                msg->SetAnnotation(key, json_object_get_string(val));
            }
        }
    }
    return msg;
}
