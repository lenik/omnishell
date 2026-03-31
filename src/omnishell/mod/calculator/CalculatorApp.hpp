#ifndef CALCULATOR_APP_HPP
#define CALCULATOR_APP_HPP

#include "../../core/Module.hpp"

class VolumeManager;

namespace os {

class App;

/**
 * Calculator Application Module
 *
 * Standard calculator with basic arithmetic operations.
 */
class CalculatorApp : public Module {
  public:
    CalculatorApp(CreateModuleContext* ctx);
    virtual ~CalculatorApp();

    ProcessPtr run(const RunConfig& config) override;

    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif // CALCULATOR_APP_HPP
