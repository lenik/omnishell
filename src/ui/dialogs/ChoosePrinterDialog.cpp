#include "ChoosePrinterDialog.hpp"

#include <wx/log.h>

namespace os {

ChoosePrinterDialog::ChoosePrinterDialog(
    wxWindow* parent,
    const wxString& title,
    const wxString& message
)
    : wxDialog(parent, wxID_ANY, title,
                wxDefaultPosition, wxSize(500, 400),
                wxDEFAULT_DIALOG_STYLE)
    , selectedIndex_(-1)
{
    CreateControls();
    
    if (!message.IsEmpty()) {
        messageText_->SetLabel(message);
    }
    
    // Add some default printers for demo
    // In production, these would come from configuration
    addPrinter("HP LaserJet Pro", "Black & White Laser", "Office - Floor 2", true);
    addPrinter("Canon PIXMA", "Color Inkjet", "Office - Floor 1");
    addPrinter("Brother MFC", "Multifunction", "Office - Floor 3");
    
    UpdatePrinterList();
}

ChoosePrinterDialog::~ChoosePrinterDialog() {
}

void ChoosePrinterDialog::CreateControls() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Message
    messageText_ = new wxStaticText(this, wxID_ANY, "Choose a printer:");
    mainSizer->Add(messageText_, 0, wxALL, 5);
    
    // Printer list
    printerList_ = new wxListCtrl(this, wxID_ANY,
                                   wxDefaultPosition, wxSize(-1, 200),
                                   wxLC_REPORT | wxLC_SINGLE_SEL);
    
    // Add columns
    printerList_->InsertColumn(0, "Printer Name", wxLIST_FORMAT_LEFT, 200);
    printerList_->InsertColumn(1, "Description", wxLIST_FORMAT_LEFT, 150);
    printerList_->InsertColumn(2, "Location", wxLIST_FORMAT_LEFT, 100);
    printerList_->InsertColumn(3, "Status", wxLIST_FORMAT_LEFT, 80);
    
    printerList_->Bind(wxEVT_LIST_ITEM_SELECTED, &ChoosePrinterDialog::OnPrinterSelected, this);
    printerList_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ChoosePrinterDialog::OnPrinterDoubleClicked, this);
    
    mainSizer->Add(printerList_, 1, wxEXPAND | wxALL, 5);
    
    // Status area
    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    
    statusText_ = new wxStaticText(this, wxID_ANY, "");
    statusSizer->Add(statusText_, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    mainSizer->Add(statusSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* okButton = new wxButton(this, wxID_OK, "OK");
    okButton->Bind(wxEVT_BUTTON, &ChoosePrinterDialog::OnOK, this);
    okButton->Enable(false);  // Disabled until printer selected
    
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    cancelButton->Bind(wxEVT_BUTTON, &ChoosePrinterDialog::OnCancel, this);
    
    setDefaultButton_ = new wxButton(this, wxID_ANY, "Set as Default");
    setDefaultButton_->Bind(wxEVT_BUTTON, &ChoosePrinterDialog::OnSetDefault, this);
    setDefaultButton_->Enable(false);
    
    wxButton* propsButton = new wxButton(this, wxID_ANY, "Properties");
    propsButton->Bind(wxEVT_BUTTON, &ChoosePrinterDialog::OnProperties, this);
    propsButton->Enable(false);
    
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(setDefaultButton_, 0, wxALL, 5);
    buttonSizer->Add(propsButton, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
    Layout();
}

void ChoosePrinterDialog::addPrinter(
    const wxString& name,
    const wxString& description,
    const wxString& location,
    bool isDefault
) {
    PrinterInfo info;
    info.name = name;
    info.description = description;
    info.location = location;
    info.isDefault = isDefault;
    info.isOnline = true;  // Assume online by default
    info.isLocal = true;   // Assume local by default
    
    printers_.push_back(info);
    
    if (isDefault) {
        defaultPrinter_ = name;
    }
}

wxString ChoosePrinterDialog::getSelectedPrinter() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)printers_.size()) {
        return printers_[selectedIndex_].name;
    }
    return wxString();
}

PrinterInfo ChoosePrinterDialog::getPrinterInfo() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)printers_.size()) {
        return printers_[selectedIndex_];
    }
    return PrinterInfo();
}

void ChoosePrinterDialog::setDefaultPrinter(const wxString& name) {
    defaultPrinter_ = name;
    
    for (auto& printer : printers_) {
        printer.isDefault = (printer.name == name);
    }
    
    UpdatePrinterList();
}

std::vector<PrinterInfo> ChoosePrinterDialog::getPrinters() const {
    return printers_;
}

void ChoosePrinterDialog::UpdatePrinterList() {
    printerList_->DeleteAllItems();
    
    for (size_t i = 0; i < printers_.size(); i++) {
        const auto& printer = printers_[i];
        
        long index = printerList_->InsertItem(i, printer.name);
        printerList_->SetItem(index, 1, printer.description);
        printerList_->SetItem(index, 2, printer.location);
        
        wxString status;
        if (!printer.isOnline) {
            status = "Offline";
        } else if (printer.isDefault) {
            status = "Default";
        } else {
            status = "Ready";
        }
        
        printerList_->SetItem(index, 3, status);
        
        // Highlight default printer
        if (printer.isDefault) {
            // Could set item background color here
        }
    }
    
    // Select default printer if exists
    if (!defaultPrinter_.IsEmpty() && selectedIndex_ == -1) {
        for (size_t i = 0; i < printers_.size(); i++) {
            if (printers_[i].name == defaultPrinter_) {
                printerList_->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                selectedIndex_ = i;
                break;
            }
        }
    }
}

void ChoosePrinterDialog::OnOK(wxCommandEvent& event) {
    if (selectedIndex_ < 0) {
        wxMessageBox("Please select a printer", "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    EndModal(wxID_OK);
}

void ChoosePrinterDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void ChoosePrinterDialog::OnPrinterSelected(wxListEvent& event) {
    selectedIndex_ = event.GetIndex();
    
    // Enable buttons
    wxWindow* okButton = FindWindowById(wxID_OK, this);
    if (okButton) {
        okButton->Enable(true);
    }
    
    setDefaultButton_->Enable(true);
    
    UpdateStatus();
    
    event.Skip();
}

void ChoosePrinterDialog::OnPrinterDoubleClicked(wxListEvent& event) {
    OnOK(event);
}

void ChoosePrinterDialog::OnSetDefault(wxCommandEvent& event) {
    if (selectedIndex_ >= 0) {
        wxString name = printers_[selectedIndex_].name;
        setDefaultPrinter(name);
        UpdateStatus();
        
        wxMessageBox(name + " is now the default printer",
                    "Printer Settings", wxOK | wxICON_INFORMATION);
    }
}

void ChoosePrinterDialog::OnProperties(wxCommandEvent& event) {
    if (selectedIndex_ >= 0) {
        const auto& printer = printers_[selectedIndex_];
        
        wxString msg = wxString::Format(
            "Printer: %s\n"
            "Description: %s\n"
            "Location: %s\n"
            "Status: %s\n"
            "Type: %s",
            printer.name,
            printer.description,
            printer.location,
            printer.isOnline ? "Online" : "Offline",
            printer.isLocal ? "Local" : "Network"
        );
        
        wxMessageBox(msg, "Printer Properties", wxOK | wxICON_INFORMATION);
    }
}

void ChoosePrinterDialog::UpdateStatus() {
    if (selectedIndex_ >= 0) {
        const auto& printer = printers_[selectedIndex_];
        wxString status = wxString::Format(
            "%s - %s (%s)",
            printer.name,
            printer.description,
            printer.isOnline ? "Ready" : "Offline"
        );
        statusText_->SetLabel(status);
    } else {
        statusText_->SetLabel("");
    }
}

} // namespace os
