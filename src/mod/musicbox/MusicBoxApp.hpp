#ifndef OMNISHELL_MOD_MUSICBOX_MUSICBOXAPP_HPP
#define OMNISHELL_MOD_MUSICBOX_MUSICBOXAPP_HPP

#include "../../core/Module.hpp"

namespace os {

class App;

class MusicBoxApp : public Module {
public:
    explicit MusicBoxApp(CreateModuleContext* ctx);
    ~MusicBoxApp() override;

    void install() override;

    void initializeMetadata();
    ProcessPtr run(const RunConfig& config) override;

private:
    App* m_app{nullptr};
};

} // namespace os

#endif // OMNISHELL_MOD_MUSICBOX_MUSICBOXAPP_HPP

