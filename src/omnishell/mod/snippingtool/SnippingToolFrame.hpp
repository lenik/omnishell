#ifndef OMNISHELL_MOD_SNIPPINGTOOL_FRAME_HPP
#define OMNISHELL_MOD_SNIPPINGTOOL_FRAME_HPP

#include "SnippingToolBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class SnippingToolFrame : public uiFrame {
public:
    SnippingToolFrame(App* app, std::string title);
    ~SnippingToolFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

    SnippingToolBody& body() { return m_body; }

private:
    SnippingToolBody m_body;
};

} // namespace os

#endif
