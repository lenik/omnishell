#ifndef OMNISHELL_CORE_RUN_CONFIG_HPP
#define OMNISHELL_CORE_RUN_CONFIG_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace os {

/**
 * Launch context for Module::run — mirrors a standalone process: argv[0], argv[1..], and environ.
 * When a module is embedded in-process, the shell fills this from App::captureLaunchContext().
 *
 * - Desktop / start menu: typically args empty; env still points at the captured process environment.
 * - File association / CLI open: args[0] is vol://<volumeIndex>/<path> on the VFS.
 */
struct RunConfig {
    /** Program path (argv[0]). */
    std::string self;
    /** Arguments (argv[1..] style; index 0 here is the first argument after self). */
    std::vector<std::string> args;
    /** Snapshot of environment at shell startup; may be nullptr if not captured. */
    const std::unordered_map<std::string, std::string>* env{nullptr};
};

} // namespace os

#endif
