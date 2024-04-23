#pragma once

#include <memory>

namespace httplib {

class Client;

};

namespace sw::redis {

class Redis;

};

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
    HttpExporter(std::string host, int port, std::string path);
    ~HttpExporter() override;

    // thread-unsafe
    bool Export(std::unique_ptr<Message>& msg) override;
    void Close() override;

private:
    std::string m_host;
    int m_port;
    std::string m_path;
    std::unique_ptr<httplib::Client> m_client;
};

class RedisExporter : public MsgExporter {
public:
    explicit RedisExporter(std::string addr);
    ~RedisExporter() override;

    // thread-unsafe
    bool Export(std::unique_ptr<Message>& msg) override;
    void Close() override;

private:
    std::string m_addr;
    std::unique_ptr<sw::redis::Redis> m_client;
};

} // namespace qalarm
