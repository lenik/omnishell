#ifndef OMNISHELL_UI_CHOOSE_PRINTER_DIALOG_HPP
#define OMNISHELL_UI_CHOOSE_PRINTER_DIALOG_HPP

#include <wx/listctrl.h>
#include <wx/print.h>
#include <wx/wx.h>

#include <string>
#include <vector>

namespace os {

/**
 * Printer Information
 */
struct PrinterInfo {
    wxString name;
    wxString description;
    wxString location;
    bool isDefault;
    bool isOnline;
    bool isLocal;
    
    PrinterInfo()
        : isDefault(false)
        , isOnline(true)
        , isLocal(true)
    {}
};

/**
 * ChoosePrinterDialog
 * 
 * Printer selection dialog with:
 * - Printer list from whitelist
 * - Printer properties display
 * - Set as default option
 * - Printer status indication
 * 
 * Note: OmniShell uses a whitelist model - only registered
 * printers appear in the dialog.
 * 
 * Usage:
 * ```cpp
 * ChoosePrinterDialog dlg(parent, "Select Printer");
 * 
 * // Add printers to whitelist (typically done by admin)
 * dlg.addPrinter("HP LaserJet", "Office Printer", "Floor 2");
 * dlg.addPrinter("Canon PIXMA", "Color Printer", "Floor 1");
 * 
 * if (dlg.ShowModal() == wxID_OK) {
 *     wxString printer = dlg.getSelectedPrinter();
 *     // Use selected printer
 * }
 * ```
 */
class ChoosePrinterDialog : public wxDialog {
public:
    /**
     * Constructor
     * @param parent Parent window
     * @param title Dialog title
     * @param message Prompt message
     */
    ChoosePrinterDialog(
        wxWindow* parent,
        const wxString& title = "Select Printer",
        const wxString& message = "Choose a printer:"
    );
    
    virtual ~ChoosePrinterDialog();
    
    /**
     * Add printer to whitelist
     * @param name Printer name
     * @param description Printer description
     * @param location Printer location
     * @param isDefault Is this the default printer?
     */
    void addPrinter(
        const wxString& name,
        const wxString& description = "",
        const wxString& location = "",
        bool isDefault = false
    );
    
    /**
     * Get selected printer name
     */
    wxString getSelectedPrinter() const;
    
    /**
     * Get selected printer info
     */
    PrinterInfo getPrinterInfo() const;
    
    /**
     * Set default printer name
     */
    void setDefaultPrinter(const wxString& name);
    
    /**
     * Get all available printers
     */
    std::vector<PrinterInfo> getPrinters() const;

protected:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnPrinterSelected(wxListEvent& event);
    void OnPrinterDoubleClicked(wxListEvent& event);
    void OnSetDefault(wxCommandEvent& event);
    void OnProperties(wxCommandEvent& event);
    
    void CreateControls();
    void UpdatePrinterList();
    void UpdateStatus();

private:
    wxListCtrl* m_printerList;
    wxStaticText* m_statusText;
    wxStaticText* m_messageText;
    wxButton* m_setDefaultButton;
    
    std::vector<PrinterInfo> m_printers;
    int m_selectedIndex;
    wxString m_defaultPrinter;
};

} // namespace os

#endif // OMNISHELL_UI_CHOOSE_PRINTER_DIALOG_HPP
