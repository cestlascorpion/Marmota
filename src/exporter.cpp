#include "exporter.h"
#include "message.h"

#include "httplib.h"
#include "sw/redis++/redis++.h"

using namespace std;
using namespace qalarm;

HttpExporter::HttpExporter(string host, int port, string path)
    : m_host(std::move(host))
    , m_port(port)
    , m_path(std::move(path))
    , m_client(make_unique<httplib::Client>(m_host.c_str(), m_port)) {
    m_client->set_tcp_nodelay(true);
    m_client->set_keep_alive(true);
    m_client->set_connection_timeout(2, 0);
    m_client->set_read_timeout(2, 0);
    m_client->set_write_timeout(2, 0);
}

HttpExporter::~HttpExporter() = default;

bool HttpExporter::Export(std::unique_ptr<Message>& msg) {
    auto body = Message::ToString(msg);
    auto resp = m_client->Post(m_path, body, "application/json");
    if (!resp) {
        return false;
    }
    if (resp->status != httplib::StatusCode::OK_200) {
        return false;
    }
    return true;
}

void HttpExporter::Close() {
    m_client->stop();
    m_client.reset(nullptr);
}

RedisExporter::RedisExporter(string addr)
    : m_addr(std::move(addr))
    , m_client(make_unique<sw::redis::Redis>(m_addr)) {
}

RedisExporter::~RedisExporter() = default;

bool RedisExporter::Export(std::unique_ptr<Message>& msg) {
    time_t now;
    time(&now);
    char ts[16]{0};
    strftime(ts, sizeof(ts), "%Y-%m-%d", std::localtime(&now));
    auto key = "alarms-" + string{ts};
    auto score = time(nullptr);

    try {
        m_client->zadd(key, Message::ToString(msg), static_cast<double>(score));
        m_client->expire(key, 86400);
    } catch (const sw::redis::Error& e) {
        return false;
    }
    return true;
}

void RedisExporter::Close() {
    m_client.reset(nullptr);
}
