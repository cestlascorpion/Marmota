#include "exporter.h"
#include "message.h"

#include "zlog.h"

using namespace std;
using namespace qalarm;

HttpExporter::HttpExporter() {
    // TODO
    dzlog_debug("HttpExporter ctor");
}

HttpExporter::~HttpExporter() {
    // TODO
    dzlog_debug("HttpExporter dctor");
}

bool HttpExporter::Export(std::unique_ptr<Message>&) {
    // TODO
    return true;
}

void HttpExporter::Close() {
    // TODO
    dzlog_debug("HttpExporter close");
}

RedisExporter::RedisExporter() {
    // TODO
    dzlog_debug("RedisExporter ctor");
}

RedisExporter::~RedisExporter() {
    // TODO
    dzlog_debug("RedisExporter dctor");
}

bool RedisExporter::Export(std::unique_ptr<Message>&) {
    // TODO
    return true;
}

void RedisExporter::Close() {
    // TODO
    dzlog_debug("RedisExporter close");
}
