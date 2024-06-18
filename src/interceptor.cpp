#include "interceptor.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>

#include <cstring>

#include "message.h"
#include "zlog.h"

using namespace std;
using namespace qalarm;

namespace detail {

constexpr const char *kAppKey = "app";
constexpr const char *kPidKey = "pid";
constexpr const char *kAddrKey = "addr";

string getPid() {
    return to_string(getpid());
}

string getApp() {
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

int getIpLinux(int ipv4_6, string &out) {
    int ret_val;
    struct ifaddrs *ifAddrStruct = nullptr;
    void *tmpAddrPtr;

    ret_val = getifaddrs(&ifAddrStruct);
    if (ret_val != 0) {
        ret_val = errno;
        return ret_val;
    }

    string str_ipvX;
    int ad_buf_len;
    char addressBuffer[INET6_ADDRSTRLEN] = {0};

    if (ipv4_6 == AF_INET6) {
        ad_buf_len = INET6_ADDRSTRLEN;
    } else {
        ad_buf_len = INET_ADDRSTRLEN;
    }

    while (ifAddrStruct != nullptr) {
        if (ipv4_6 == ifAddrStruct->ifa_addr->sa_family) {
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            inet_ntop(ipv4_6, tmpAddrPtr, addressBuffer, static_cast<socklen_t>(ad_buf_len));
            str_ipvX = string(addressBuffer);
            if (!out.empty()) {
                out.append(",");
            }
            out.append(str_ipvX);
            memset(addressBuffer, 0, static_cast<size_t>(ad_buf_len));
        }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    return ret_val;
}

string getIpv4Linux() {
    string out;
    auto ret = getIpLinux(AF_INET, out);
    if (ret != 0) {
        return "unknown";
    }
    return out;
}

} // namespace detail

MsgFiller::MsgFiller()
    : m_app(detail::getApp())
    , m_pid(detail::getPid())
    , m_addr(detail::getIpv4Linux()) {}

MsgFiller::~MsgFiller() = default;

void MsgFiller::Intercept(unique_ptr<Message> &msg) {
    msg->SetAnnotation(detail::kAppKey, m_app);
    msg->SetAnnotation(detail::kPidKey, m_pid);
    msg->SetAnnotation(detail::kAddrKey, m_addr);
}

void MsgFiller::Close() {
    // do nothing
}

MsgPrinter::MsgPrinter() = default;

MsgPrinter::~MsgPrinter() = default;

void MsgPrinter::Intercept(unique_ptr<Message> &msg) {
    dzlog_debug("MsgPrinter %s", Message::ToString(msg).c_str());
}

void MsgPrinter::Close() {
    // do nothing
}