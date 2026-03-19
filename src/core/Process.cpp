#include "Process.hpp"

#include <chrono>

namespace os {

Process::Process() {
    // cheap unique id: time + pointer
    auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now())
                   .time_since_epoch()
                   .count();
    id = std::to_string(static_cast<long long>(now)) + "-" +
         std::to_string(reinterpret_cast<uintptr_t>(this));
}

void Process::addWindow(wxWindow* w) {
    if (!w)
        return;
    m_windows.push_back(w);
    if (m_onWindowAdded) {
        m_onWindowAdded(shared_from_this(), w);
    }
}

} // namespace os

