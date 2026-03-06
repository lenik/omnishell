#include "Module.hpp"

#include <wx/log.h>

namespace os {

Module::Module() 
    : runCount(0)
{
}

Module::~Module() {
}

bool Module::isEnabled() const {
    return true;
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
    return uri.empty() ? name : (uri + "." + name);
}

void Module::recordExecution() {
    lastRunTime = wxDateTime::Now();
    runCount++;
}

} // namespace os
