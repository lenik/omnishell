#ifndef OMNISHELL_CORE_SERVICE_MANAGER_HPP
#define OMNISHELL_CORE_SERVICE_MANAGER_HPP

#include "Module.hpp"

#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace os {

class ModuleRegistry;

/**
 * Service Manager
 * 
 * Manages lifecycle of service modules:
 * - Automatic startup
 * - Monitoring and restart on failure
 * - Graceful shutdown
 * - Thread management for thread-owned services
 */
class ServiceManager {
public:
    struct ServiceState {
        ModulePtr module;
        std::thread thread;
        std::atomic<bool> running;
        std::atomic<bool> shouldStop;
        int restartCount;
        wxDateTime lastStartTime;
        wxDateTime lastStopTime;
        
        ServiceState() 
            : running(false)
            , shouldStop(false)
            , restartCount(0)
        {}
    };
    
    /**
     * Get singleton instance
     */
    static ServiceManager& getInstance();
    
    /**
     * Start a service module
     * @param module Service module to start
     * @param autoRestart Whether to automatically restart on failure
     */
    void startService(ModulePtr module, bool autoRestart = true);
    
    /**
     * Stop a service module
     * @param uri Module URI
     * @param wait Whether to wait for thread to finish
     */
    void stopService(const std::string& uri, bool wait = true);
    
    /**
     * Stop all services
     * @param wait Whether to wait for threads to finish
     */
    void stopAllServices(bool wait = true);
    
    /**
     * Check if service is running
     */
    bool isServiceRunning(const std::string& uri) const;
    
    /**
     * Get all running services
     */
    std::vector<std::string> getRunningServices() const;
    
    /**
     * Get service state
     */
    ServiceState* getServiceState(const std::string& uri);
    
    /**
     * Set maximum restart attempts
     */
    void setMaxRestarts(int maxRestarts);
    
    /**
     * Initialize and start all registered service modules
     */
    void startAllServices(ModuleRegistry& registry);

private:
    ServiceManager() = default;
    ~ServiceManager();
    
    // Prevent copying
    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;
    
    void runServiceThread(ServiceState& state);
    void handleServiceFailure(ServiceState& state);
    
    std::map<std::string, std::unique_ptr<ServiceState>> services_;
    mutable std::mutex mutex_;
    int maxRestarts_ = 3;
};

} // namespace os

#endif // OMNISHELL_CORE_SERVICE_MANAGER_HPP
