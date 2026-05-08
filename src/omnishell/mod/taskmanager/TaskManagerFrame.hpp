#ifndef OMNISHELL_MOD_TASKMANAGER_FRAME_HPP
#define OMNISHELL_MOD_TASKMANAGER_FRAME_HPP

#include "TaskManagerBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class TaskManagerFrame : public uiFrame {
public:
    TaskManagerFrame(App* app, std::string title);
    ~TaskManagerFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

    TaskManagerBody& body() { return m_body; }

private:
    TaskManagerBody m_body;
};

} // namespace os

#endif
