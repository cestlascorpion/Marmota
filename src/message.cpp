#include "message.h"

using namespace std;
using namespace qalarm;

Message::Message(MsgLevel level, uint32_t code, string desc, MsgKvType annot)
    : m_level(level)
    , m_code(code)
    , m_desc(std::move(desc))
    , m_annotation(std::move(annot))
    , m_tp(chrono::steady_clock::now()) {
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
    // TODO: JSON string
    return "";
}

std::unique_ptr<Message> Message::FromString(const string &str) {
    // TODO：JSON string
    return nullptr;
}
