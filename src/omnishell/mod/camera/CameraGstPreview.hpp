#ifndef OMNISHELL_MOD_CAMERA_CAMERA_GST_PREVIEW_HPP
#define OMNISHELL_MOD_CAMERA_CAMERA_GST_PREVIEW_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace os {

/**
 * Live camera preview via GStreamer (v4l2src → RGB → appsink).
 * Optional: only built when GStreamer is available.
 */
class CameraGstPreview {
public:
    CameraGstPreview() = default;
    ~CameraGstPreview();

    CameraGstPreview(const CameraGstPreview&) = delete;
    CameraGstPreview& operator=(const CameraGstPreview&) = delete;

    bool start(const std::string& devicePath, int width, int height);
    void stop();
    bool isRunning() const;

    /** Non-blocking pull; returns false if no frame yet. RGB 8-bit, row-major. */
    bool tryPullRgbFrame(std::vector<uint8_t>& outRgb, int& outW, int& outH);

private:
    void* m_pipeline{nullptr};
    void* m_appsink{nullptr};
};

} // namespace os

#endif
