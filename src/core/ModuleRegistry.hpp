#ifndef OMNISHELL_CORE_MODULE_REGISTRY_HPP
#define OMNISHELL_CORE_MODULE_REGISTRY_HPP

#include "Module.hpp"

#include <functional>
#include <map>
#include <vector>

namespace os {

/**
 * Module Registry - Central repository for all registered modules
 * 
 * Provides:
 * - Module registration and discovery
 * - Module lookup by URI or name
 * - Module enumeration for UI (desktop, start menu)
 * - Lifecycle management coordination
 */
class ModuleRegistry {
public:
    using ModuleFactory = std::function<ModulePtr()>;
    
    struct ModuleInfo {
        ModuleFactory factory;
        ModulePtr instance;
        bool isInstalled;
    };
    
    /**
     * Get singleton instance
     */
    static ModuleRegistry& getInstance();
    
    /**
     * Register a module type with the registry
     * 
     * @param uri Module URI (e.g., "omnishell.notepad")
     * @param factory Factory function to create module instances
     */
    void registerModule(const std::string& uri, ModuleFactory factory);
    
    /**
     * Unregister a module
     */
    void unregisterModule(const std::string& uri);
    
    /**
     * Get module by URI
     * @return Module instance or nullptr if not found
     */
    ModulePtr getModule(const std::string& uri);
    
    /**
     * Get or create module instance
     * Creates instance if it doesn't exist
     */
    ModulePtr getOrCreateModule(const std::string& uri);
    
    /**
     * Get all registered modules
     */
    std::vector<ModulePtr> getAllModules() const;
    
    /**
     * Get visible modules (for UI display)
     */
    std::vector<ModulePtr> getVisibleModules() const;
    
    /**
     * Get enabled modules
     */
    std::vector<ModulePtr> getEnabledModules() const;
    
    /**
     * Get service modules
     */
    std::vector<ModulePtr> getServiceModules() const;
    
    /**
     * Search modules by name/description
     */
    std::vector<ModulePtr> searchModules(const std::string& query) const;
    
    /**
     * Check if module is registered
     */
    bool hasModule(const std::string& uri) const;
    
    /**
     * Install all registered modules
     */
    void installAll();
    
    /**
     * Uninstall all modules
     */
    void uninstallAll();
    
    /**
     * Start all service modules
     */
    void startServices();
    
    /**
     * Stop all service modules
     */
    void stopServices();

private:
    ModuleRegistry() = default;
    ~ModuleRegistry();
    
    // Prevent copying
    ModuleRegistry(const ModuleRegistry&) = delete;
    ModuleRegistry& operator=(const ModuleRegistry&) = delete;
    
    std::map<std::string, ModuleInfo> modules_;
};

} // namespace os

// Module registration macro
#define OMNISHELL_REGISTER_MODULE(uri, module_class) \
    namespace { \
        struct module_class##_Registrar { \
            module_class##_Registrar() { \
                os::ModuleRegistry::getInstance().registerModule( \
                    uri, \
                    []() -> os::ModulePtr { \
                        return std::make_shared<module_class>(); \
                    } \
                ); \
            } \
        }; \
        static module_class##_Registrar module_class##_registrar; \
    }

#endif // OMNISHELL_CORE_MODULE_REGISTRY_HPP
