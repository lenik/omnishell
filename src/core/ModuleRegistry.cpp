#include "ModuleRegistry.hpp"

#include <wx/log.h>

#include <algorithm>

namespace os {

ModuleRegistry& ModuleRegistry::getInstance() {
    static ModuleRegistry instance;
    return instance;
}

ModuleRegistry::~ModuleRegistry() {
    uninstallAll();
}

void ModuleRegistry::registerModule(const std::string& uri, ModuleFactory factory) {
    if (modules_.find(uri) != modules_.end()) {
        wxLogWarning("Module already registered: %s", uri);
        return;
    }
    
    ModuleInfo info;
    info.factory = factory;
    info.instance = nullptr;
    info.isInstalled = false;
    modules_[uri] = info;
    
    wxLogInfo("Module registered: %s", uri);
}

void ModuleRegistry::unregisterModule(const std::string& uri) {
    auto it = modules_.find(uri);
    if (it != modules_.end()) {
        if (it->second.isInstalled && it->second.instance) {
            it->second.instance->uninstall();
        }
        modules_.erase(it);
        wxLogInfo("Module unregistered: %s", uri);
    }
}

ModulePtr ModuleRegistry::getModule(const std::string& uri) {
    auto it = modules_.find(uri);
    if (it != modules_.end()) {
        return it->second.instance;
    }
    return nullptr;
}

ModulePtr ModuleRegistry::getOrCreateModule(const std::string& uri) {
    auto it = modules_.find(uri);
    if (it == modules_.end()) {
        return nullptr;
    }
    
    if (!it->second.instance) {
        it->second.instance = it->second.factory();
        if (!it->second.instance) {
            wxLogError("Failed to create module instance: %s", uri);
            return nullptr;
        }
    }
    
    return it->second.instance;
}

std::vector<ModulePtr> ModuleRegistry::getAllModules() const {
    std::vector<ModulePtr> result;
    result.reserve(modules_.size());
    
    for (const auto& pair : modules_) {
        if (pair.second.instance) {
            result.push_back(pair.second.instance);
        }
    }
    
    return result;
}

std::vector<ModulePtr> ModuleRegistry::getVisibleModules() const {
    std::vector<ModulePtr> result;
    
    for (const auto& pair : modules_) {
        if (pair.second.instance && pair.second.instance->isVisible()) {
            result.push_back(pair.second.instance);
        }
    }
    
    return result;
}

std::vector<ModulePtr> ModuleRegistry::getEnabledModules() const {
    std::vector<ModulePtr> result;
    
    for (const auto& pair : modules_) {
        if (pair.second.instance && pair.second.instance->isEnabled()) {
            result.push_back(pair.second.instance);
        }
    }
    
    return result;
}

std::vector<ModulePtr> ModuleRegistry::getServiceModules() const {
    std::vector<ModulePtr> result;
    
    for (const auto& pair : modules_) {
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
    
    for (const auto& pair : modules_) {
        if (!pair.second.instance) continue;
        
        const auto& module = pair.second.instance;
        if (!module->isVisible()) continue;
        
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
    return modules_.find(uri) != modules_.end();
}

void ModuleRegistry::installAll() {
    for (auto& pair : modules_) {
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
    for (auto& pair : modules_) {
        if (pair.second.isInstalled && pair.second.instance) {
            pair.second.instance->uninstall();
            pair.second.isInstalled = false;
        }
    }
    wxLogInfo("All modules uninstalled");
}

void ModuleRegistry::startServices() {
    for (const auto& pair : modules_) {
        if (pair.second.instance && pair.second.instance->isService()) {
            wxLogInfo("Starting service: %s", pair.first);
            // Services may run in separate threads based on isThreadOwned()
            if (pair.second.instance->isThreadOwned()) {
                // TODO: Implement thread management
                wxLogInfo("Service %s is thread-owned (not yet implemented)", pair.first);
            } else {
                // Run in UI thread
                pair.second.instance->run();
            }
        }
    }
}

void ModuleRegistry::stopServices() {
    for (const auto& pair : modules_) {
        if (pair.second.instance && pair.second.instance->isService()) {
            wxLogInfo("Stopping service: %s", pair.first);
            // Services should handle cleanup in their run loop
        }
    }
}

} // namespace os
