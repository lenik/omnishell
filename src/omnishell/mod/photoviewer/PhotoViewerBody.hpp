#ifndef OMNISHELL_MOD_PHOTO_VIEWER_BODY_HPP
#define OMNISHELL_MOD_PHOTO_VIEWER_BODY_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/volume/VolumeFile.hpp>

#include <wx/image.h>
#include <wx/panel.h>

#include <optional>
#include <string>
#include <vector>

namespace os {

class PhotoViewerBody : public UIFragment {
public:
    PhotoViewerBody();
    ~PhotoViewerBody() override = default;

    void createFragmentView(CreateViewContext* ctx) override;

    void loadVolumeFile(const VolumeFile& file);

private:
    wxPanel* m_root{nullptr};
    wxStaticBitmap* m_view{nullptr};

    wxImage m_original;

    std::optional<VolumeFile> m_file;
    std::string m_dirPath;
    std::vector<std::string> m_dirImages; // absolute paths within the same volume
    int m_dirIndex{-1};

    void updateShownBitmap();
    void refreshDirImageList();
    void showDirIndex(int index);

    void onPrev(PerformContext*);
    void onNext(PerformContext*);
    void onDelete(PerformContext*);
};

} // namespace os

#endif // OMNISHELL_MOD_PHOTO_VIEWER_BODY_HPP

