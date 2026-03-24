#ifndef OMNISHELL_MOD_CONSOLE_APP_HPP
#define OMNISHELL_MOD_CONSOLE_APP_HPP

#include "../../core/Module.hpp"

namespace os {

class App;

class ConsoleApp : public Module {
public:
    explicit ConsoleApp(CreateModuleContext* ctx);
    ~ConsoleApp() override;

    ProcessPtr run(const RunConfig& config) override;

    void initializeMetadata();

private:
    App* m_app;
};

} // namespace os

#endif
