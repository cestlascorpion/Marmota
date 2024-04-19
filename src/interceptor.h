#pragma once

#include <memory>
#include <string>
#include <map>

#include "message.h"

namespace qalarm {

class MsgInterceptor {
public:
    virtual ~MsgInterceptor() = default;

    virtual void Intercept(std::unique_ptr<Message> &msg) = 0;
    virtual void Close() = 0;
};

class MsgFiller : public MsgInterceptor {
public:
    MsgFiller();
    ~MsgFiller() override;

    // thead-safe
    void Intercept(std::unique_ptr<Message> &msg) override;
    void Close() override;

private:
    const std::string m_app;
    const std::string m_pid;
    const std::string m_addr;
};

class MsgPrinter : public MsgInterceptor {
public:
    MsgPrinter();
    ~MsgPrinter() override;

    // thead-safe
    void Intercept(std::unique_ptr<Message> &msg) override;
    void Close() override;
};

} // namespace qalarm
