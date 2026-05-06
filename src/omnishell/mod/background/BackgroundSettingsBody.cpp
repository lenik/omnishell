#include "BackgroundSettingsBody.hpp"

#include "../../core/RegistryDb.hpp"
#include "../../shell/Shell.hpp"
#include "../../ui/dialogs/ChooseFileDialog.hpp"

#include <bas/volume/VolumeFile.hpp>

#include <wx/button.h>
#include <wx/mstream.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace os {

namespace {
enum {
    ID_GROUP_BG = uiFrame::ID_APP_HIGHEST + 50,
    ID_CHOOSE_IMAGE,
    ID_APPLY_COLOR,
    ID_APPLY_IMAGE,
};
}

BackgroundSettingsBody::BackgroundSettingsBody() {
    const std::string dir = "heroicons/normal";

    group(ID_GROUP_BG, "settings", "background", 1000, "&Background", "Desktop background settings").install();

    int seq = 0;
    action(ID_CHOOSE_IMAGE, "settings/background", "choose_image", seq++, "&Choose Image...", "Select background image")
        .icon(wxART_FILE_OPEN, dir, "photo.svg")
        .performFn([this](PerformContext* ctx) { onChooseImage(ctx); })
        .install();
    action(ID_APPLY_COLOR, "settings/background", "apply_color", seq++, "Apply &Color", "Apply solid color")
        .icon(wxART_TIP, dir, "swatch.svg")
        .performFn([this](PerformContext* ctx) { onApplyColor(ctx); })
        .install();
    action(ID_APPLY_IMAGE, "settings/background", "apply_image", seq++, "Apply &Image", "Apply selected image")
        .icon(wxART_TIP, dir, "photo.svg")
        .performFn([this](PerformContext* ctx) { onApplyImage(ctx); })
        .install();
}

void BackgroundSettingsBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    m_root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* title = new wxStaticText(m_root, wxID_ANY, "Desktop background");
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    sizer->Add(title, 0, wxALL, 12);

    wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(m_root, wxID_ANY, "Color:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_picker = new wxColourPickerCtrl(m_root, wxID_ANY, *wxLIGHT_GREY);
    row1->Add(m_picker, 0, wxALIGN_CENTER_VERTICAL);
    row1->AddStretchSpacer();
    wxButton* applyColorBtn = new wxButton(m_root, wxID_ANY, "Apply Color");
    row1->Add(applyColorBtn, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(row1, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    wxBoxSizer* row2 = new wxBoxSizer(wxHORIZONTAL);
    row2->Add(new wxStaticText(m_root, wxID_ANY, "Image:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    wxButton* chooseBtn = new wxButton(m_root, wxID_ANY, "Choose...");
    row2->Add(chooseBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    wxButton* applyImageBtn = new wxButton(m_root, wxID_ANY, "Apply Image");
    applyImageBtn->Disable();
    row2->Add(applyImageBtn, 0, wxALIGN_CENTER_VERTICAL);
    row2->AddStretchSpacer();
    sizer->Add(row2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    wxStaticText* hint = new wxStaticText(
        m_root, wxID_ANY,
        "Tip: image uses default volume. Settings are saved and auto-loaded on startup.");
    hint->Wrap(520);
    sizer->Add(hint, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    applyColorBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        auto pc = toPerformContext(e);
        onApplyColor(&pc);
    });
    chooseBtn->Bind(wxEVT_BUTTON, [this, applyImageBtn](wxCommandEvent& e) {
        auto pc = toPerformContext(e);
        onChooseImage(&pc);
        applyImageBtn->Enable(!m_selectedImagePath.empty());
    });
    applyImageBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        auto pc = toPerformContext(e);
        onApplyImage(&pc);
    });

    m_root->SetSizer(sizer);
}

void BackgroundSettingsBody::onChooseImage(PerformContext*) {
    auto shell = ShellApp::getInstance();
    if (!shell || !shell->getVolumeManager() || !m_frame)
        return;

    ChooseFileDialog dlg(m_frame, shell->getVolumeManager(), "Select Background Image", FileDialogMode::Open, "/");
    dlg.addFilter("Images", "*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.webp");
    dlg.setFileMustExist(true);
    if (dlg.ShowModal() != wxID_OK)
        return;
    auto vf = dlg.getVolumeFile();
    if (!vf)
        return;
    m_selectedImagePath = vf->getPath();
    if (vf->getVolume())
        m_selectedImageVolumeId = vf->getVolume()->getUUID();
    else
        m_selectedImageVolumeId.clear();
}

void BackgroundSettingsBody::onApplyColor(PerformContext*) {
    auto shell = ShellApp::getInstance();
    if (!shell || !shell->getDesktopWindow() || !m_picker)
        return;

    wxColour c = m_picker->GetColour();
    char buf[16];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", (int)c.Red(), (int)c.Green(), (int)c.Blue());

    RegistryDb::getInstance().set("Desktop.Background.Mode", "color");
    RegistryDb::getInstance().set("Desktop.Background.Color", buf);
    RegistryDb::getInstance().save();
    shell->getDesktopWindow()->setBackgroundColor(c);
}

void BackgroundSettingsBody::onApplyImage(PerformContext*) {
    auto shell = ShellApp::getInstance();
    if (!shell || !shell->getDesktopWindow() || !shell->getVolumeManager())
        return;
    if (m_selectedImagePath.empty())
        return;

    Volume* vol = nullptr;
    if (!m_selectedImageVolumeId.empty()) {
        for (size_t i = 0; i < shell->getVolumeManager()->getVolumeCount(); ++i) {
            Volume* v = shell->getVolumeManager()->getVolume(i);
            if (v && v->getUUID() == m_selectedImageVolumeId) {
                vol = v;
                break;
            }
        }
    }
    if (!vol)
        vol = shell->getVolumeManager()->getDefaultVolume();
    if (!vol)
        return;

    try {
        VolumeFile vf(vol, m_selectedImagePath);
        auto data = vf.readFile();
        if (data.empty())
            return;
        wxMemoryInputStream ms(data.data(), data.size());
        wxImage img(ms, wxBITMAP_TYPE_ANY);
        if (!img.IsOk())
            return;

        RegistryDb::getInstance().set("Desktop.Background.Mode", "image");
        RegistryDb::getInstance().set("Desktop.Background.ImagePath", m_selectedImagePath);
        RegistryDb::getInstance().set("Desktop.Background.ImageVolumeId", vol->getUUID());
        RegistryDb::getInstance().save();
        shell->getDesktopWindow()->setBackgroundImage(wxBitmap(img));
    } catch (...) {
    }
}

} // namespace os

