#include "interceptor.h"

#include <unistd.h>
#include <string.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "zlog.h"

using namespace std;
using namespace qalarm;

namespace detail {

string get_pid() {
    return to_string(getpid());
}

string get_app() {
    string proc{"unknown"};
    char path[1024] = {0};
    if (readlink("/proc/self/exe", path, sizeof(path) - 1) > 0) {
        char *path_end;
        path_end = strrchr(path, '/');
        if (path_end != nullptr) {
            ++path_end;
            proc = path_end;
        }
    }
    return proc;
}

int get_ip_linux(int ipv4_6, string &out) {
    int ret_val = 0;
    struct ifaddrs *ifAddrStruct = nullptr;
    void *tmpAddrPtr = nullptr;

    ret_val = getifaddrs(&ifAddrStruct);
    if (ret_val != 0) {
        ret_val = errno;
        return ret_val;
    }

    string str_ipvX;
    int padress_buf_len = 0;
    char addressBuffer[INET6_ADDRSTRLEN] = {0};

    if (ipv4_6 == AF_INET6) {
        padress_buf_len = INET6_ADDRSTRLEN;
    } else {
        padress_buf_len = INET_ADDRSTRLEN;
    }

    while (ifAddrStruct != nullptr) {
        if (ipv4_6 == ifAddrStruct->ifa_addr->sa_family) {
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            inet_ntop(ipv4_6, tmpAddrPtr, addressBuffer, padress_buf_len);
            str_ipvX = string(addressBuffer);
            if (!out.empty()) {
                out.append(",");
            }
            out.append(str_ipvX);
            memset(addressBuffer, 0, padress_buf_len);
        }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    return ret_val;
}

string get_ipv4_linux() {
    string out;
    auto ret = get_ip_linux(AF_INET, out);
    if (ret != 0) {
        return "unknown";
    }
    return out;
}

} // namespace detail

MsgFiller::MsgFiller()
    : m_app(detail::get_app())
    , m_pid(detail::get_pid())
    , m_addr(detail::get_ipv4_linux()) {
    dzlog_debug("MsgFiller ctor %s %s %s", m_app.c_str(), m_pid.c_str(), m_addr.c_str());
}

MsgFiller::~MsgFiller() {
    dzlog_debug("MsgFill dctor");
}

void MsgFiller::Intercept(unique_ptr<Message> &msg) {
    msg->SetAnnotation(kAppKey, m_app);
    msg->SetAnnotation(kPidKey, m_pid);
    msg->SetAnnotation(kAddrKey, m_addr);
}

void MsgFiller::Close() {
    // do nothing
    dzlog_debug("MsgFill close");
}

MsgPrinter::MsgPrinter() {
    dzlog_debug("MsgPrinter ctor");
}

MsgPrinter::~MsgPrinter() {
    dzlog_debug("MsgPrinter dctor");
}

void MsgPrinter::Intercept(unique_ptr<Message> &msg) {
    dzlog_debug("MsgPrinter %s", Message::ToString(msg).c_str());
}

void MsgPrinter::Close() {
    // do nothing
    dzlog_debug("MsgPrinter close");
}