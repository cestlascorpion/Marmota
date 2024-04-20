#pragma once

#include <memory>

namespace qalarm {

class Message;

class MsgExporter {
public:
    virtual ~MsgExporter() = default;

    virtual bool Export(std::unique_ptr<Message>& msg) = 0;
    virtual void Close() = 0;
};

class HttpExporter : public MsgExporter {
public:
    HttpExporter();
    ~HttpExporter() override;

    bool Export(std::unique_ptr<Message>& msg) override;
    void Close() override;
};

class RedisExporter : public MsgExporter {
public:
    RedisExporter();
    ~RedisExporter() override;

    bool Export(std::unique_ptr<Message>& msg) override;
    void Close() override;
};

} // namespace qalarm
