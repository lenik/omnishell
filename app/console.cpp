/**
 * Standalone OmniShell Console (Terminal + zash).
 *
 * Initializes global os::app, VolumeManager, then wx + ModuleRegistry and runs ConsoleApp.
 */

#include <omnishell/core/App.hpp>
#include <omnishell/core/ModuleRegistry.hpp>
#include <omnishell/mod/console/ConsoleApp.hpp>

#include <bas/proc/env.hpp>
#include <bas/proc/stackdump.h>
#include <bas/volume/LocalVolume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <bas/wx/app.hpp>

#include <wx/log.h>

#if defined(__unix__) || defined(__APPLE__)
#include <getopt.h>
#endif

namespace os {

class ConsoleWxApp : public uiApp {
public:
    ConsoleWxApp() = default;

    ~ConsoleWxApp() override {
        if (m_moduleRegistry) {
            m_moduleRegistry->uninstallAll();
            delete m_moduleRegistry;
            m_moduleRegistry = nullptr;
        }
    }

    bool OnUserInit() override {
        if (!app.volumeManager) {
            wxLogError("VolumeManager not initialized");
            return false;
        }

        SetAppName("Console");
        SetAppDisplayName("OmniShell Console");
        SetVendorName("OmniShell Project");

        m_moduleRegistry = new ModuleRegistry(&app);
        m_moduleRegistry->installAll();

        ModulePtr module = m_moduleRegistry->getOrCreateModule(kConsoleUri);
        if (!module) {
            wxLogError("Module not found: %s", kConsoleUri);
            return false;
        }

        try {
            module->recordExecution();
            (void)module->run(app.makeRunConfig());
        } catch (const std::exception& e) {
            wxLogError("Failed to start console: %s", e.what());
            return false;
        }

        return true;
    }

private:
    static constexpr const char* kConsoleUri = "omnishell.Console";

    ModuleRegistry* m_moduleRegistry{nullptr};
};

} // namespace os

int main(int argc, char** argv) {
    stackdump_install_crash_handler(&stackdump_color_schema_default);
    stackdump_set_interactive(1);

    os::app.captureLaunchContext(argc, argv);

    os::app.volumeManager = std::make_unique<VolumeManager>();
    os::app.startFiles.clear();

#if defined(__unix__) || defined(__APPLE__)
    static struct option longopts[] = {{"open", required_argument, nullptr, 'o'},
                                         {"local", required_argument, nullptr, 'l'},
                                         {nullptr, 0, nullptr, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "l:o:", longopts, nullptr)) != -1) {
        switch (opt) {
        case 'l':
            if (optarg && optarg[0]) {
                os::app.volumeManager->addVolume(std::make_unique<LocalVolume>(optarg));
            }
            break;
        case 'o':
            if (optarg)
                os::app.startFiles.push_back(optarg);
            break;
        default:
            break;
        }
    }
#endif

    os::app.addDefaultLocalVolumesIfEmpty();

    os::ConsoleWxApp wxApp;
    return wxApp.main(argc, argv);
}
