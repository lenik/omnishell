#include "ChoosePrinterDialog.hpp"

#include <wx/log.h>

namespace os {

ChoosePrinterDialog::ChoosePrinterDialog(
    wxWindow* parent,
    const wxString& title,
    const wxString& message
)
    : wxcDialog(parent, wxID_ANY, title,
                wxDefaultPosition, wxSize(500, 400),
                wxDEFAULT_DIALOG_STYLE)
    , m_selectedIndex(-1)
{
    CreateControls();
    
    if (!message.IsEmpty()) {
        m_messageText->SetLabel(message);
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
    m_messageText = new wxStaticText(this, wxID_ANY, "Choose a printer:");
    mainSizer->Add(m_messageText, 0, wxALL, 5);
    
    // Printer list
    m_printerList = new wxListCtrl(this, wxID_ANY,
                                   wxDefaultPosition, wxSize(-1, 200),
                                   wxLC_REPORT | wxLC_SINGLE_SEL);
    
    // Add columns
    m_printerList->InsertColumn(0, "Printer Name", wxLIST_FORMAT_LEFT, 200);
    m_printerList->InsertColumn(1, "Description", wxLIST_FORMAT_LEFT, 150);
    m_printerList->InsertColumn(2, "Location", wxLIST_FORMAT_LEFT, 100);
    m_printerList->InsertColumn(3, "Status", wxLIST_FORMAT_LEFT, 80);
    
    m_printerList->Bind(wxEVT_LIST_ITEM_SELECTED, &ChoosePrinterDialog::OnPrinterSelected, this);
    m_printerList->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ChoosePrinterDialog::OnPrinterDoubleClicked, this);
    
    mainSizer->Add(m_printerList, 1, wxEXPAND | wxALL, 5);
    
    // Status area
    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_statusText = new wxStaticText(this, wxID_ANY, "");
    statusSizer->Add(m_statusText, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    mainSizer->Add(statusSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* okButton = new wxButton(this, wxID_OK, "OK");
    okButton->Bind(wxEVT_BUTTON, &ChoosePrinterDialog::OnOK, this);
    okButton->Enable(false);  // Disabled until printer selected
    
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    cancelButton->Bind(wxEVT_BUTTON, &ChoosePrinterDialog::OnCancel, this);
    
    m_setDefaultButton = new wxButton(this, wxID_ANY, "Set as Default");
    m_setDefaultButton->Bind(wxEVT_BUTTON, &ChoosePrinterDialog::OnSetDefault, this);
    m_setDefaultButton->Enable(false);
    
    wxButton* propsButton = new wxButton(this, wxID_ANY, "Properties");
    propsButton->Bind(wxEVT_BUTTON, &ChoosePrinterDialog::OnProperties, this);
    propsButton->Enable(false);
    
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_setDefaultButton, 0, wxALL, 5);
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
    
    m_printers.push_back(info);
    
    if (isDefault) {
        m_defaultPrinter = name;
    }
}

wxString ChoosePrinterDialog::getSelectedPrinter() const {
    if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_printers.size()) {
        return m_printers[m_selectedIndex].name;
    }
    return wxString();
}

PrinterInfo ChoosePrinterDialog::getPrinterInfo() const {
    if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_printers.size()) {
        return m_printers[m_selectedIndex];
    }
    return PrinterInfo();
}

void ChoosePrinterDialog::setDefaultPrinter(const wxString& name) {
    m_defaultPrinter = name;
    
    for (auto& printer : m_printers) {
        printer.isDefault = (printer.name == name);
    }
    
    UpdatePrinterList();
}

std::vector<PrinterInfo> ChoosePrinterDialog::getPrinters() const {
    return m_printers;
}

void ChoosePrinterDialog::UpdatePrinterList() {
    m_printerList->DeleteAllItems();
    
    for (size_t i = 0; i < m_printers.size(); i++) {
        const auto& printer = m_printers[i];
        
        long index = m_printerList->InsertItem(i, printer.name);
        m_printerList->SetItem(index, 1, printer.description);
        m_printerList->SetItem(index, 2, printer.location);
        
        wxString status;
        if (!printer.isOnline) {
            status = "Offline";
        } else if (printer.isDefault) {
            status = "Default";
        } else {
            status = "Ready";
        }
        
        m_printerList->SetItem(index, 3, status);
        
        // Highlight default printer
        if (printer.isDefault) {
            // Could set item background color here
        }
    }
    
    // Select default printer if exists
    if (!m_defaultPrinter.IsEmpty() && m_selectedIndex == -1) {
        for (size_t i = 0; i < m_printers.size(); i++) {
            if (m_printers[i].name == m_defaultPrinter) {
                m_printerList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                m_selectedIndex = i;
                break;
            }
        }
    }
}

void ChoosePrinterDialog::OnOK(wxCommandEvent& event) {
    if (m_selectedIndex < 0) {
        wxMessageBox("Please select a printer", "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    EndModal(wxID_OK);
}

void ChoosePrinterDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void ChoosePrinterDialog::OnPrinterSelected(wxListEvent& event) {
    m_selectedIndex = event.GetIndex();
    
    // Enable buttons
    wxWindow* okButton = FindWindowById(wxID_OK, this);
    if (okButton) {
        okButton->Enable(true);
    }
    
    m_setDefaultButton->Enable(true);
    
    UpdateStatus();
    
    event.Skip();
}

void ChoosePrinterDialog::OnPrinterDoubleClicked(wxListEvent& event) {
    OnOK(event);
}

void ChoosePrinterDialog::OnSetDefault(wxCommandEvent& event) {
    if (m_selectedIndex >= 0) {
        wxString name = m_printers[m_selectedIndex].name;
        setDefaultPrinter(name);
        UpdateStatus();
        
        wxMessageBox(name + " is now the default printer",
                    "Printer Settings", wxOK | wxICON_INFORMATION);
    }
}

void ChoosePrinterDialog::OnProperties(wxCommandEvent& event) {
    if (m_selectedIndex >= 0) {
        const auto& printer = m_printers[m_selectedIndex];
        
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
    if (m_selectedIndex >= 0) {
        const auto& printer = m_printers[m_selectedIndex];
        wxString status = wxString::Format(
            "%s - %s (%s)",
            printer.name,
            printer.description,
            printer.isOnline ? "Ready" : "Offline"
        );
        m_statusText->SetLabel(status);
    } else {
        m_statusText->SetLabel("");
    }
}

} // namespace os
