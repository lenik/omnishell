#include "CameraGstPreview.hpp"

#if defined(OMNISHELL_HAVE_GSTREAMER) && OMNISHELL_HAVE_GSTREAMER

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <cstring>

namespace os {

namespace {

static void ensureGstInit() {
    static bool done = false;
    if (!done) {
        gst_init(nullptr, nullptr);
        done = true;
    }
}

} // namespace

CameraGstPreview::~CameraGstPreview() { stop(); }

bool CameraGstPreview::start(const std::string& devicePath, int width, int height) {
    stop();
    ensureGstInit();

    const std::string dev = devicePath.empty() ? "/dev/video0" : devicePath;
    (void)width;
    (void)height;
    // Preserve device aspect ratio: don't force output width/height in the caps filter.
    std::string desc = "v4l2src device=" + dev +
                        " ! videoconvert ! video/x-raw,format=RGB"
                        " ! appsink name=sink emit-signals=false sync=false max-buffers=2 drop=true";

    GError* err = nullptr;
    GstElement* pipe = gst_parse_launch(desc.c_str(), &err);
    if (!pipe) {
        if (err)
            g_error_free(err);
        return false;
    }

    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    if (!sink || !GST_IS_APP_SINK(sink)) {
        if (sink)
            gst_object_unref(sink);
        gst_object_unref(pipe);
        return false;
    }

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    m_pipeline = static_cast<void*>(pipe);
    m_appsink = static_cast<void*>(sink);
    return true;
}

void CameraGstPreview::stop() {
#if defined(OMNISHELL_HAVE_GSTREAMER) && OMNISHELL_HAVE_GSTREAMER
    if (m_appsink) {
        gst_object_unref(GST_APP_SINK(m_appsink));
        m_appsink = nullptr;
    }
    if (m_pipeline) {
        gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_NULL);
        gst_object_unref(GST_ELEMENT(m_pipeline));
        m_pipeline = nullptr;
    }
#endif
}

bool CameraGstPreview::isRunning() const {
    return m_pipeline != nullptr;
}

bool CameraGstPreview::tryPullRgbFrame(std::vector<uint8_t>& outRgb, int& outW, int& outH) {
#if defined(OMNISHELL_HAVE_GSTREAMER) && OMNISHELL_HAVE_GSTREAMER
    if (!m_appsink)
        return false;

    GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(m_appsink), 0);
    if (!sample)
        return false;

    GstCaps* caps = gst_sample_get_caps(sample);
    GstStructure* st = caps ? gst_caps_get_structure(caps, 0) : nullptr;
    int w = 0, h = 0;
    if (!st || !gst_structure_get_int(st, "width", &w) || !gst_structure_get_int(st, "height", &h) ||
        w <= 0 || h <= 0) {
        gst_sample_unref(sample);
        return false;
    }

    constexpr int kMaxDim = 4096;
    if (w > kMaxDim || h > kMaxDim) {
        gst_sample_unref(sample);
        return false;
    }
    const size_t needCheck = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    if (needCheck / (static_cast<size_t>(w) * 3u) != static_cast<size_t>(h)) {
        gst_sample_unref(sample);
        return false;
    }

    GstBuffer* buf = gst_sample_get_buffer(sample);
    if (!buf) {
        gst_sample_unref(sample);
        return false;
    }

    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return false;
    }

    const size_t need = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    outRgb.resize(need);
    if (map.size >= need)
        std::memcpy(outRgb.data(), map.data, need);
    else {
        std::memcpy(outRgb.data(), map.data, map.size);
        if (map.size < need)
            std::memset(outRgb.data() + map.size, 0, need - map.size);
    }

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);

    outW = w;
    outH = h;
    return true;
#else
    (void)outRgb;
    (void)outW;
    (void)outH;
    return false;
#endif
}

} // namespace os

#else

namespace os {

CameraGstPreview::~CameraGstPreview() = default;

bool CameraGstPreview::start(const std::string&, int, int) { return false; }

void CameraGstPreview::stop() {}

bool CameraGstPreview::isRunning() const { return false; }

bool CameraGstPreview::tryPullRgbFrame(std::vector<uint8_t>&, int&, int&) { return false; }

} // namespace os

#endif
