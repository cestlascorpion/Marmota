#include <unistd.h>
#include <csignal>
#include <atomic>
#include <memory>

#include "alarmer.h"
#include "interceptor.h"
#include "exporter.h"

#include "zlog.h"

using namespace std;
using namespace qalarm;

atomic<bool> running;

void signal_handler(int sig) {
    dzlog_info("catch signal: %d", sig);
    zlog_fini();
    running.store(false);
    // exit(sig);
}

shared_ptr<Alarmer> GetDefaultAlarmer() {
    vector<unique_ptr<MsgInterceptor>> interceptors;
    auto mfInter = make_unique<MsgFiller>();
    interceptors.push_back(std::move(mfInter));
    auto mpInter = make_unique<MsgPrinter>();
    interceptors.push_back(std::move(mpInter));

    vector<unique_ptr<MsgExporter>> exporters;
    auto hExp = make_unique<HttpExporter>("0.0.0.0", 8080, "/alarm/report");
    exporters.push_back(std::move(hExp));
    auto rExp = make_unique<RedisExporter>("tcp://127.0.0.1:6379");
    exporters.push_back(std::move(rExp));

    return make_shared<Alarmer>(std::move(interceptors), std::move(exporters));
}

int main(/*int argc, char **argv*/) {
    auto ret = dzlog_init("zlog.conf", "test");
    if (ret != 0) {
        printf("dzlog_init failed\n");
        return 0;
    }
    signal(SIGTERM, signal_handler); // kill -15 PID

    auto alarmer = GetDefaultAlarmer();

    running.store(true);
    while (running.load()) {
        dzlog_debug("hello, zlog");
        alarmer->AlarmFatal(ZLOG_LEVEL_FATAL, "fatal");
        alarmer->AlarmError(ZLOG_LEVEL_ERROR, "error");
        alarmer->AlarmWarn(ZLOG_LEVEL_WARN, "warn");
        alarmer->AlarmNotice(ZLOG_LEVEL_NOTICE, "notice");
        alarmer->AlarmInfo(ZLOG_LEVEL_INFO, "info");
        alarmer->AlarmDebug(ZLOG_LEVEL_DEBUG, "debug");
        sleep(1);
    }
    return 0;
}