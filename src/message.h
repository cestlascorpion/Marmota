#pragma once

#include <string>
#include <memory>
#include <map>

#include "zlog.h"

namespace qalarm {

constexpr const char *kAppKey = "app";
constexpr const char *kPidKey = "pid";
constexpr const char *kAddrKey = "addr";

enum class MsgLevel : uint32_t {
    FATAL = ZLOG_LEVEL_FATAL,
    ERROR = ZLOG_LEVEL_ERROR,
    WARN = ZLOG_LEVEL_WARN,
    NOTICE = ZLOG_LEVEL_NOTICE,
    INFO = ZLOG_LEVEL_INFO,
    DEBUG = ZLOG_LEVEL_DEBUG,
};

using MsgCoType = uint32_t;
using MsgTpType = time_t;
using MsgKvType = std::map<std::string, std::string>;

using MsgIdType = uint64_t;

class Message {
public:
    Message(MsgLevel level, MsgCoType code, std::string desc, MsgKvType annot = MsgKvType());
    Message(MsgLevel level, MsgCoType code, std::string desc, MsgTpType tp, MsgKvType annot = MsgKvType());

    Message(const Message &) = delete;
    Message &operator=(const Message &) = delete;
    Message(Message &&) = delete;
    Message &operator=(Message &&) = delete;

public:
    MsgIdType GeMsgId() const;
    MsgLevel GetMsgLevel() const;

    void SetAnnotation(const std::string &k, std::string v);
    std::string GetAnnotation(const std::string &k) const;

public:
    static std::string ToString(std::unique_ptr<Message> &msg);
    static std::unique_ptr<Message> FromString(const std::string &str);

private:
    MsgLevel m_lv;
    MsgCoType m_co;
    MsgTpType m_tp;
    std::string m_desc;
    std::map<std::string, std::string> m_annotation;
};

} // namespace qalarm
