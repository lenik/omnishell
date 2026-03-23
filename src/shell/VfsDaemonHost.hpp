#ifndef OMNISHELL_SHELL_VFS_DAEMON_HOST_HPP
#define OMNISHELL_SHELL_VFS_DAEMON_HOST_HPP

#include "daemon/HttpDaemon.hpp"

#include <memory>
#include <string>

class VolumeManager;

namespace os {

/**
 * Owns the process-wide HttpDaemon (HTTP + optional HTTPS) for VFS browsing APIs.
 * Started from ShellApp after VolumeManager is ready; stopped on shell shutdown.
 */
class VfsDaemonHost {
  public:
    VfsDaemonHost() = default;
    ~VfsDaemonHost();

    VfsDaemonHost(const VfsDaemonHost&) = delete;
    VfsDaemonHost& operator=(const VfsDaemonHost&) = delete;

    bool start(VolumeManager* vm);
    void stop();

    bool isRunning() const;
    const std::string& httpBase() const { return m_httpBase; }
    /** Empty if HTTPS thread was not started. */
    const std::string& httpsBase() const { return m_httpsBase; }

    /** Path inside volume without leading slash (e.g. dir/track.mp3). */
    std::string httpUrlForVolumePath(size_t volumeIndex, const std::string& pathInVolume) const;
    /** Path after /asset/ without leading slash. */
    std::string httpUrlForAssetPath(const std::string& pathInAsset) const;

  private:
    std::unique_ptr<HttpDaemon> m_daemon;
    std::string m_httpBase;
    std::string m_httpsBase;
};

} // namespace os

#endif
