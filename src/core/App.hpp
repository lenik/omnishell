#ifndef OMNISHELL_CORE_APP_HPP
#define OMNISHELL_CORE_APP_HPP

#include <memory>
#include <string>
#include <vector>

class VolumeManager;

namespace os {

class App {
public:
    std::unique_ptr<VolumeManager> volumeManager;
    std::vector<std::string> startFiles;

    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;
};

extern App app;

} // namespace os

#endif

