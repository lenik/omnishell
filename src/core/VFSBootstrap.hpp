#ifndef OMNISHELL_CORE_VFS_BOOTSTRAP_HPP
#define OMNISHELL_CORE_VFS_BOOTSTRAP_HPP

#include <memory>
#include <string>

class VolumeManager;

namespace os {

/**
 * VFS bootstrap from command line (VFS_README).
 * Call before wxEntry. Parses -l/--local, -u/--user, -p/--password, --dump, --open.
 * Builds VolumeManager; if --dump, dumps and returns false (caller should exit).
 */
bool initVFSFromCommandLine(int argc, char* argv[]);

/** Get the VolumeManager built by initVFSFromCommandLine. Never null after init (default volume added if no -l). */
VolumeManager* getVolumeManager();

/** Optional path to open at startup (--open <path>). Empty if not set. */
const std::string& getOpenPath();

/** Free VolumeManager (call from app shutdown if desired). */
void shutdownVFS();

} // namespace os

#endif // OMNISHELL_CORE_VFS_BOOTSTRAP_HPP
