#include "ServiceManager.hpp"

#include "ModuleRegistry.hpp"

#include <wx/log.h>
#include <wx/timer.h>

namespace os {

ServiceManager& ServiceManager::getInstance() {
    static ServiceManager instance;
    return instance;
}

ServiceManager::~ServiceManager() {
    stopAllServices(true);
}

void ServiceManager::startService(ModulePtr module, bool autoRestart) {
    if (!module) {
        wxLogError("Cannot start null service");
        return;
    }
    
    if (!module->isService()) {
        wxLogWarning("Module %s is not a service", module->getFullUri());
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    const std::string uri = module->getFullUri();
    
    // Check if already running
    auto it = services_.find(uri);
    if (it != services_.end() && it->second->running) {
        wxLogWarning("Service %s is already running", uri);
        return;
    }
    
    // Create service state
    auto state = std::make_unique<ServiceState>();
    state->module = module;
    state->running = true;
    state->shouldStop = false;
    state->lastStartTime = wxDateTime::Now();
    
    // Install if not already installed
    if (!module->installTime.IsValid()) {
        module->install();
    }
    
    wxLogInfo("Starting service: %s", uri);
    
    // Start service thread if thread-owned
    if (module->isThreadOwned()) {
        state->thread = std::thread(&ServiceManager::runServiceThread, this, std::ref(*state));
    } else {
        // Run in UI thread - just call run() once
        try {
            module->run();
        } catch (const std::exception& e) {
            wxLogError("Service %s threw exception: %s", uri, e.what());
            if (autoRestart) {
                handleServiceFailure(*state);
            }
        }
        state->running = false;
        state->lastStopTime = wxDateTime::Now();
    }
    
    services_[uri] = std::move(state);
}

void ServiceManager::stopService(const std::string& uri, bool wait) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(uri);
    if (it == services_.end()) {
        wxLogWarning("Service %s not found", uri);
        return;
    }
    
    ServiceState& state = *it->second;
    
    if (!state.running) {
        wxLogWarning("Service %s is not running", uri);
        return;
    }
    
    wxLogInfo("Stopping service: %s", uri);
    
    // Signal thread to stop
    state.shouldStop = true;
    
    if (state.module && state.module->isThreadOwned()) {
        // Wait for thread to finish
        if (wait && state.thread.joinable()) {
            state.thread.join();
        } else if (state.thread.joinable()) {
            state.thread.detach();
        }
    }
    
    state.running = false;
    state.lastStopTime = wxDateTime::Now();
    
    if (!wait) {
        services_.erase(it);
    }
}

void ServiceManager::stopAllServices(bool wait) {
    std::vector<std::string> uris;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : services_) {
            if (pair.second->running) {
                uris.push_back(pair.first);
            }
        }
    }
    
    for (const auto& uri : uris) {
        stopService(uri, wait);
    }
    
    wxLogInfo("All services stopped");
}

bool ServiceManager::isServiceRunning(const std::string& uri) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(uri);
    if (it == services_.end()) {
        return false;
    }
    
    return it->second->running;
}

std::vector<std::string> ServiceManager::getRunningServices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    for (const auto& pair : services_) {
        if (pair.second->running) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

ServiceManager::ServiceState* ServiceManager::getServiceState(const std::string& uri) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(uri);
    if (it == services_.end()) {
        return nullptr;
    }
    
    return it->second.get();
}

void ServiceManager::setMaxRestarts(int maxRestarts) {
    maxRestarts_ = maxRestarts;
}

void ServiceManager::startAllServices() {
    auto& registry = ModuleRegistry::getInstance();
    auto services = registry.getServiceModules();
    
    wxLogInfo("Starting %zu service modules", services.size());
    
    for (auto& service : services) {
        startService(service);
    }
}

void ServiceManager::runServiceThread(ServiceState& state) {
    const std::string uri = state.module->getFullUri();
    wxLogInfo("Service thread started: %s", uri);
    
    while (!state.shouldStop && state.running) {
        try {
            state.module->run();
        } catch (const std::exception& e) {
            wxLogError("Service %s threw exception: %s", uri, e.what());
            
            if (state.shouldStop) {
                break;
            }
            
            handleServiceFailure(state);
            
            if (!state.running) {
                break;
            }
        }
        
        // Small delay to prevent tight loop
        wxThread::Sleep(100);
    }
    
    state.running = false;
    state.lastStopTime = wxDateTime::Now();
    wxLogInfo("Service thread stopped: %s", uri);
}

void ServiceManager::handleServiceFailure(ServiceState& state) {
    state.restartCount++;
    
    if (state.restartCount >= maxRestarts_) {
        wxLogError("Service %s exceeded max restart attempts (%d), stopping", 
                   state.module->getFullUri(), maxRestarts_);
        state.running = false;
        return;
    }
    
    wxLogWarning("Service %s restarting (attempt %d/%d)", 
                 state.module->getFullUri(), state.restartCount, maxRestarts_);
    
    // Brief delay before restart
    wxThread::Sleep(1000);
}

} // namespace os
