#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <atomic>

#include "src/alarm.h"
#include "src/interceptor.h"
#include "src/exporter.h"

#include "zlog.h"

using namespace std;

atomic<bool> running;

void signal_handler(int sig) {
    dzlog_info("catch signal: %d", sig);
    zlog_fini();
    running.store(false);
    // exit(sig);
}

int main(/*int argc, char **argv*/) {
    auto ret = dzlog_init("zlog.conf", "test");
    if (ret != 0) {
        printf("dzlog_init failed\n");
        return 0;
    }
    signal(SIGTERM, signal_handler); // kill -15 PID

    vector<unique_ptr<qalarm::MsgInterceptor>> interceptors;
    auto mfInter = unique_ptr<qalarm::MsgFiller>(new qalarm::MsgFiller());
    interceptors.push_back(std::move(mfInter));

    vector<unique_ptr<qalarm::MsgExporter>> exporters;
    auto hExp = unique_ptr<qalarm::HttpExporter>(new qalarm::HttpExporter());
    auto rExp = unique_ptr<qalarm::RedisExporter>(new qalarm::RedisExporter());

    auto m = make_shared<qalarm::Alarm>(std::move(interceptors), std::move(exporters));

    running.store(true);
    while (running.load()) {
        dzlog_debug("hello, zlog");
        m->AlarmFatal(ZLOG_LEVEL_FATAL, "fatal");
        m->AlarmError(ZLOG_LEVEL_ERROR, "error");
        m->AlarmWarn(ZLOG_LEVEL_WARN, "warn");
        m->AlarmNotice(ZLOG_LEVEL_NOTICE, "notice");
        m->AlarmInfo(ZLOG_LEVEL_INFO, "info");
        m->AlarmDebug(ZLOG_LEVEL_DEBUG, "debug");
        sleep(1);
    }
    return 0;
}