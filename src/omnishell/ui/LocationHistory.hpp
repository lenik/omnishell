#ifndef LOCATIONHISTORY_H
#define LOCATIONHISTORY_H

#include "Location.hpp"

#include <bas/volume/Volume.hpp>

#include <functional>
#include <vector>

class LocationHistory {
public:
    explicit LocationHistory(const Volume* volume, std::string_view path);

    /** Go to path on current volume: no-op if same, else add to history and notify. */
    void go(const Volume* volume, std::string_view path);

    /** Current location as (volume, path). */
    Location location() const { return m_history[m_historyIndex]; }

    void goBack();
    void goForward();
    void goUp();
    void goHome();

    bool canGoBack() const;
    bool canGoForward() const;
    bool canGoUp() const;

    /** Callback receives (volume, path) so UI can switch volume when navigating history. */
    void onLocationChange(std::function<void(const Location& location)> callback);

private:
    bool pathExists(const Volume* vol, std::string_view path) const;
    void addToHistory(const Volume* volume, std::string_view path);
    void notify();

private:
    std::vector<Location> m_history;
    size_t m_historyIndex;
    std::vector<std::function<void(const Location& location)>> m_locationChangeListeners;

};

#endif // LOCATIONHISTORY_H