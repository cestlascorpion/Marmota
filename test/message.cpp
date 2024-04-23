#include <iostream>
#include <memory>

#include "message.h"

#include "zlog.h"

using namespace std;

int main() {
    auto ret = dzlog_init("zlog.conf", "test");
    if (ret != 0) {
        printf("dzlog_init failed\n");
        return 0;
    }

    auto msg = make_unique<qalarm::Message>(qalarm::MsgLevel::DEBUG, 1, "hello, zlog",
                                            map<string, string>{{"k1", "v1"}, {"k2", "v2"}});
    auto str = qalarm::Message::ToString(msg);
    cout << str << endl;
    auto gsm = qalarm::Message::FromString(str);
    cout << qalarm::Message::ToString(gsm) << endl;
    zlog_fini();
}