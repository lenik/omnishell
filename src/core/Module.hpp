#ifndef OMNISHELL_CORE_MODULE_HPP
#define OMNISHELL_CORE_MODULE_HPP

#include <bas/ui/arch/ImageSet.hpp>

#include <wx/datetime.h>

#include <memory>
#include <string>

namespace os {

/**
 * Base class for all OmniShell modules (applications, services, tools)
 * 
 * Modules are the fundamental unit of functionality in OmniShell.
 * They can be:
 * - Standard applications (launched by user)
 * - Service modules (run automatically in background)
 * - Configuration tools
 * - System utilities
 */
class Module {
public:
    // Metadata fields
    std::string uri;           // Module namespace identifier
    std::string name;          // Scripting name
    std::string label;         // Display name
    std::string description;   // Short description
    std::string doc;           // Documentation/help text
    ImageSet image;            // Icon set
    
    // Runtime tracking fields (persisted to .cache/<app>/)
    wxDateTime installTime;    // Installation timestamp
    wxDateTime lastRunTime;    // Last execution time
    int runCount;              // Total run count
    
    Module();
    virtual ~Module();
    
    // === Visibility and State ===
    
    /**
     * Check if module is enabled and can be executed
     * Override to implement custom enable logic
     */
    virtual bool isEnabled() const;
    
    /**
     * Check if module is visible in UI (desktop, start menu, search)
     * Override to hide modules from UI
     */
    virtual bool isVisible() const;
    
    // === Execution Model ===
    
    /**
     * Check if this is a service module
     * Services are automatically started by the shell
     * @return true if this is a background service
     */
    virtual bool isService() const;
    
    /**
     * Check if module runs in a separate thread
     * @return true if module is thread-owned
     */
    virtual bool isThreadOwned() const;
    
    // === Lifecycle Methods ===
    
    /**
     * Initialize module resources
     * Called once when module is installed/loaded
     * 
     * Override to:
     * - Create configuration
     * - Register tray icons
     * - Initialize services
     */
    virtual void install();
    
    /**
     * Cleanup module resources
     * Called when module is uninstalled/unloaded
     * 
     * Override to:
     * - Remove settings
     * - Unregister services
     * - Free resources
     */
    virtual void uninstall();
    
    /**
     * Entry point for module execution
     * Called when user launches the module or service starts
     * 
     * Override to:
     * - Open main window (applications)
     * - Start service loop (services)
     */
    virtual void run() = 0;
    
    // === Utility Methods ===
    
    /**
     * Get full qualified identifier
     */
    std::string getFullUri() const;
    
    /**
     * Update last run time and increment run count
     */
    void recordExecution();
};

using ModulePtr = std::shared_ptr<Module>;

} // namespace os

#endif // OMNISHELL_CORE_MODULE_HPP
