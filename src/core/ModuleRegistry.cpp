#include "ModuleRegistry.hpp"

#include <wx/log.h>

#include <algorithm>
#include <chrono>
#include <thread>

namespace os {

namespace {

class CreateModuleContextImpl : public CreateModuleContext {
  public:
    explicit CreateModuleContextImpl(VolumeManager* volumeManager)
        : m_volumeManager(volumeManager) {}

    VolumeManager* getVolumeManager() const override { return m_volumeManager; }

  private:
    VolumeManager* m_volumeManager;
};

// Global registry of module factories. This is intentionally separate from
// ModuleRegistry instances so that modules can self-register via static
// initializers without requiring a global ModuleRegistry singleton.
std::map<std::string, ModuleRegistry::ModuleFactory>& globalModuleFactories() {
    static std::map<std::string, ModuleRegistry::ModuleFactory> factories;
    return factories;
}

} // namespace

ModuleRegistry::ModuleRegistry(VolumeManager* volumeManager)
    : m_volumeManager(volumeManager) {
    // Initialize registry entries from globally registered factories.
    for (const auto& pair : globalModuleFactories()) {
        ModuleInfo info;
        info.factory = pair.second;
        info.instance = nullptr;
        info.isInstalled = false;
        m_modules[pair.first] = info;
    }
}

ModuleRegistry::~ModuleRegistry() { uninstallAll(); }

void ModuleRegistry::registerModuleFactory(const std::string& uri, ModuleFactory factory) {
    auto& factories = globalModuleFactories();
    if (factories.find(uri) != factories.end()) {
        wxLogWarning("Module factory already registered: %s", uri);
        return;
    }
    factories[uri] = std::move(factory);
    wxLogInfo("Module factory registered: %s", uri);
}

void ModuleRegistry::unregisterModule(const std::string& uri) {
    auto it = m_modules.find(uri);
    if (it != m_modules.end()) {
        if (it->second.isInstalled && it->second.instance) {
            it->second.instance->uninstall();
        }
        m_modules.erase(it);
        wxLogInfo("Module unregistered: %s", uri);
    }
}

ModulePtr ModuleRegistry::getModule(const std::string& uri) {
    auto it = m_modules.find(uri);
    if (it != m_modules.end()) {
        return it->second.instance;
    }
    return nullptr;
}

ModulePtr ModuleRegistry::getOrCreateModule(const std::string& uri) {
    auto it = m_modules.find(uri);
    if (it == m_modules.end()) {
        return nullptr;
    }

    if (!it->second.instance) {
        CreateModuleContextImpl ctx(m_volumeManager);
        it->second.instance = it->second.factory(&ctx);
        if (!it->second.instance) {
            wxLogError("Failed to create module instance: %s", uri);
            return nullptr;
        }
    }

    return it->second.instance;
}

std::vector<ModulePtr> ModuleRegistry::getAllModules() const {
    std::vector<ModulePtr> result;
    result.reserve(m_modules.size());

    for (const auto& pair : m_modules) {
        if (pair.second.instance) {
            result.push_back(pair.second.instance);
        }
    }

    return result;
}

std::vector<ModulePtr> ModuleRegistry::getVisibleModules() const {
    std::vector<ModulePtr> result;

    for (const auto& pair : m_modules) {
        if (pair.second.instance && pair.second.instance->isVisible()) {
            result.push_back(pair.second.instance);
        }
    }

    return result;
}

std::vector<ModulePtr> ModuleRegistry::getEnabledModules() const {
    std::vector<ModulePtr> result;

    for (const auto& pair : m_modules) {
        if (pair.second.instance && pair.second.instance->isEnabled()) {
            result.push_back(pair.second.instance);
        }
    }

    return result;
}

std::vector<ModulePtr> ModuleRegistry::getServiceModules() const {
    std::vector<ModulePtr> result;

    for (const auto& pair : m_modules) {
        if (pair.second.instance && pair.second.instance->isService()) {
            result.push_back(pair.second.instance);
        }
    }

    return result;
}

std::vector<ModulePtr> ModuleRegistry::searchModules(const std::string& query) const {
    std::vector<ModulePtr> result;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    for (const auto& pair : m_modules) {
        if (!pair.second.instance)
            continue;

        const auto& module = pair.second.instance;
        if (!module->isVisible())
            continue;

        // Search in name, label, and description
        std::string name = module->name;
        std::string label = module->label;
        std::string desc = module->description;

        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::transform(label.begin(), label.end(), label.begin(), ::tolower);
        std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);

        if (name.find(lowerQuery) != std::string::npos ||
            label.find(lowerQuery) != std::string::npos ||
            desc.find(lowerQuery) != std::string::npos) {
            result.push_back(module);
        }
    }

    return result;
}

bool ModuleRegistry::hasModule(const std::string& uri) const {
    return m_modules.find(uri) != m_modules.end();
}

void ModuleRegistry::installAll() {
    for (auto& pair : m_modules) {
        if (!pair.second.isInstalled) {
            auto module = getOrCreateModule(pair.first);
            if (module) {
                module->install();
                pair.second.isInstalled = true;
            }
        }
    }
    wxLogInfo("All modules installed");
}

void ModuleRegistry::uninstallAll() {
    for (auto& pair : m_modules) {
        if (pair.second.isInstalled && pair.second.instance) {
            pair.second.instance->uninstall();
            pair.second.isInstalled = false;
        }
    }
    wxLogInfo("All modules uninstalled");
}

void ModuleRegistry::startServices() {
    for (const auto& pair : m_modules) {
        if (pair.second.instance && pair.second.instance->isService()) {
            wxLogInfo("Starting service: %s", pair.first);
            // Services may run in separate threads based on isThreadOwned()
            if (pair.second.instance->isThreadOwned()) {
                std::thread([m = pair.second.instance]() {
                    try {
                        (void)m->run();
                    } catch (...) {
                    }
                }).detach();
            } else {
                // Run in UI thread
                (void)pair.second.instance->run();
            }
        }
    }
}

void ModuleRegistry::stopServices() {
    for (const auto& pair : m_modules) {
        if (pair.second.instance && pair.second.instance->isService()) {
            wxLogInfo("Stopping service: %s", pair.first);
            // Services should handle cleanup in their run loop
        }
    }
}

} // namespace os
