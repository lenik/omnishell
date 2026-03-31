#include "Module.hpp"

#include "RegistryDb.hpp"

#include <wx/log.h>

namespace os {

Module::Module(CreateModuleContext* ctx) 
    : categoryId(ID_CATEGORY_NONE)
    , runCount(0)
{
}

Module::~Module() {
}

bool Module::isEnabled() const {
    // Configurable enable/disable via RegistryDb.
    // Key convention: Module.Disabled.<fullUri> = "1"
    const std::string key = "Module.Disabled." + getFullUri();
    return RegistryDb::getInstance().getString(key, "0") != "1";
}

bool Module::isVisible() const {
    return true;
}

bool Module::isService() const {
    return false;
}

bool Module::isThreadOwned() const {
    return false;
}

void Module::install() {
    installTime = wxDateTime::Now();
    wxLogInfo("Module installed: %s", label);
}

void Module::uninstall() {
    wxLogInfo("Module uninstalled: %s", label);
}

std::string Module::getFullUri() const {
    return !uri.empty() ? uri : name;
}

void Module::recordExecution() {
    lastRunTime = wxDateTime::Now();
    runCount++;
}

} // namespace os
