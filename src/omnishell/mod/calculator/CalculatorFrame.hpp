#ifndef OMNISHELL_MOD_CALCULATOR_FRAME_HPP
#define OMNISHELL_MOD_CALCULATOR_FRAME_HPP

#include "CalculatorBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class CalculatorFrame : public uiFrame {
public:
    CalculatorFrame(App* app, std::string title);
    ~CalculatorFrame() override = default;

    void createFragmentView(CreateViewContext* ctx) override;

    CalculatorBody& body() { return m_body; }

private:
    CalculatorBody m_body;
};

} // namespace os

#endif
