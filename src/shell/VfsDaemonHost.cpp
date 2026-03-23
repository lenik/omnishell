#include "VfsDaemonHost.hpp"

#include <bas/volume/VolumeManager.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cstdio>
#include <sstream>

namespace os {

namespace {

bool tcpPortAvailable(const std::string& host, int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return false;
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        close(s);
        return false;
    }
    const bool ok = bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
    close(s);
    return ok;
}

int pickHttpPort(const std::string& host, int basePort, int maxTries) {
    for (int i = 0; i < maxTries; ++i) {
        const int p = basePort + i;
        if (tcpPortAvailable(host, p))
            return p;
    }
    return -1;
}

int pickSecondPort(const std::string& host, int avoidPort, int basePort, int maxTries) {
    for (int i = 0; i < maxTries; ++i) {
        const int p = basePort + i;
        if (p == avoidPort)
            continue;
        if (tcpPortAvailable(host, p))
            return p;
    }
    return -1;
}

std::string encodeUriPath(std::string_view path) {
    std::string out;
    out.reserve(path.size() + 8);
    for (unsigned char c : path) {
        if (std::isalnum(c) || c == '/' || c == '-' || c == '.' || c == '_')
            out += static_cast<char>(c);
        else {
            char hex[5];
            std::snprintf(hex, sizeof(hex), "%%%02X", c);
            out += hex;
        }
    }
    return out;
}

} // namespace

VfsDaemonHost::~VfsDaemonHost() {
    stop();
}

bool VfsDaemonHost::isRunning() const {
    return m_daemon != nullptr && m_daemon->isRunning();
}

bool VfsDaemonHost::start(VolumeManager* vm) {
    stop();
    if (!vm)
        return false;

    constexpr int kHttpBase = 4380;
    constexpr int kHttpsBase = 4381;
    constexpr int kSpan = 64;

    const int httpPort = pickHttpPort("127.0.0.1", kHttpBase, kSpan);
    if (httpPort < 0)
        return false;
    const int httpsPort = pickSecondPort("127.0.0.1", httpPort, kHttpsBase, kSpan);
    if (httpsPort < 0)
        return false;

    try {
        m_daemon = std::make_unique<HttpDaemon>(vm, "127.0.0.1", httpPort, httpsPort);
    } catch (...) {
        m_daemon.reset();
        return false;
    }

    if (!m_daemon->start(true)) {
        m_daemon.reset();
        return false;
    }

    m_httpBase = "http://127.0.0.1:" + std::to_string(httpPort);
    if (m_daemon->getHttpsPort() > 0)
        m_httpsBase = "https://127.0.0.1:" + std::to_string(m_daemon->getHttpsPort());
    else
        m_httpsBase.clear();

    return true;
}

void VfsDaemonHost::stop() {
    if (m_daemon) {
        m_daemon->stop();
        m_daemon.reset();
    }
    m_httpBase.clear();
    m_httpsBase.clear();
}

std::string VfsDaemonHost::httpUrlForVolumePath(size_t volumeIndex, const std::string& pathInVolume) const {
    if (m_httpBase.empty())
        return {};
    std::string p = pathInVolume;
    while (!p.empty() && p.front() == '/')
        p.erase(p.begin());
    std::string tail = p.empty() ? "" : ("/" + encodeUriPath(p));
    return m_httpBase + "/volume/" + std::to_string(volumeIndex) + tail;
}

std::string VfsDaemonHost::httpUrlForAssetPath(const std::string& pathInAsset) const {
    if (m_httpBase.empty())
        return {};
    std::string p = pathInAsset;
    while (!p.empty() && p.front() == '/')
        p.erase(p.begin());
    if (p.empty())
        return m_httpBase + "/asset/";
    return m_httpBase + "/asset/" + encodeUriPath(p);
}

} // namespace os
