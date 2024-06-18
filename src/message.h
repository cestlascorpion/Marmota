#pragma once

#include <map>
#include <memory>
#include <string>

namespace qalarm {

enum class MsgLevel : uint32_t {
    FATAL,
    ERROR,
    WARN,
    NOTICE,
    INFO,
    DEBUG,
};

class Message {
public:
    Message(MsgLevel level, uint32_t code, std::string desc,
            std::map<std::string, std::string> annot = std::map<std::string, std::string>());
    Message(MsgLevel level, uint32_t code, std::string desc, time_t tp,
            std::map<std::string, std::string> annot = std::map<std::string, std::string>());

    Message(const Message &) = delete;
    Message &operator=(const Message &) = delete;
    Message(Message &&) = delete;
    Message &operator=(Message &&) = delete;

public:
    [[nodiscard]] uint64_t GeMsgId() const;
    [[nodiscard]] MsgLevel GetMsgLevel() const;

    void SetAnnotation(const std::string &k, std::string v);

public:
    static std::string ToString(std::unique_ptr<Message> &msg);
    static std::unique_ptr<Message> FromString(const std::string &str);

private:
    MsgLevel m_lv;
    uint32_t m_co;
    time_t m_tp;
    std::string m_desc;
    std::map<std::string, std::string> m_annotation;
};

} // namespace qalarm
