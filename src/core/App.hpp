#ifndef OMNISHELL_CORE_APP_HPP
#define OMNISHELL_CORE_APP_HPP

#include "RunConfig.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../ui/IconTheme.hpp"

class VolumeManager;

namespace os {

class App {
  public:
    std::unique_ptr<VolumeManager> volumeManager;
    std::vector<std::string> startFiles;

    /** argv[0] captured at startup (for RunConfig::self). */
    std::string programSelf;
    /** Environment snapshot at startup (pointer target of RunConfig::env). */
    std::unordered_map<std::string, std::string> runtimeEnv;

    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void parseOptions(int argc, char* argv[]);

    /** If no volumes yet: optional dev testdrive, then addLocalVolumes(). */
    void addDefaultLocalVolumesIfEmpty();

    /**
     * Record argv[0] and copy the process environment into runtimeEnv.
     * Call once from main() before other argv parsing mutates the process.
     */
    void captureLaunchContext(int argc, char** argv);

    /** Build a RunConfig for module.run(); env points at runtimeEnv. */
    RunConfig makeRunConfig(std::vector<std::string> args = {}) const;

    /** Icon theme loaded from embedded assets (see assets/themes/popular.json). */
    const IconTheme* getIconTheme() const;

  private:
    mutable IconTheme m_iconTheme;
    mutable bool m_iconThemeLoaded{false};
};

extern App app;

} // namespace os

#endif
