#include "headers/fontdialog.h"
#include "../../resource.h"
#include <Uxtheme.h>
//#include <dwmapi.h>

static GlobalParams* m;
 
// Function prototypes
static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

std::string selectedFont = "Error";

std::string ShowFontDialog(GlobalParams* m0, HWND hwndModal) {
    m = m0;
    // Create the main dialog
    int id = DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(FontDialog), hwndModal, (DLGPROC)DialogProc);
    if (id == 1) {
        return selectedFont;
    }
    return "Error";
}

struct FontST {
    std::string fontNAME;
    std::string fontFILE;
};


void LoadupFonts(std::vector<FontST>* fonts) {

    HKEY hKey;
    const char* path = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD index = 0;
        char valueName[256];
        char data[256];
        DWORD valueNameSize, dataSize, type;

        // Enumerate through all the values in the key
        while (true) {
            valueNameSize = sizeof(valueName);
            dataSize = sizeof(data);
            type = 0;

            LONG ret = RegEnumValueA(hKey, index, valueName, &valueNameSize, NULL, &type, (LPBYTE)data, &dataSize);

            if (ret == ERROR_NO_MORE_ITEMS) {
                break; // No more items. GREAT!
            }
            else if (ret == ERROR_SUCCESS) {
                if (type == REG_SZ) {

                    std::string fontName = valueName;
                    size_t pos = fontName.find(" (");
                    if (pos != std::string::npos) {
                        fontName = fontName.substr(0, pos);
                    }

                    std::string str = data;
                    bool isAcceptable = false;
                    {
                        const std::string suffix = ".ttf";
                        if (str.length() >= suffix.length()) {
                            bool is = str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
                            if (is) {
                                isAcceptable = true;

                            }
                        }
                    }
                    {
                        const std::string suffix = ".TTF";
                        if (str.length() >= suffix.length()) {
                            bool is = str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
                            if (is) {
                                isAcceptable = true;

                            }
                        }
                    }
                    size_t found = str.find('\\');
                    if (found != std::string::npos) {
                        isAcceptable = false;
                    }

                    if (isAcceptable) {
                        fonts->push_back({ fontName, data });
                    }
                }
            }
            
            else {
                Beep(4000, 1000);
                MessageBox(m->hwnd, "Error reading windows registry for font names", "Error", MB_OK | MB_ICONERROR);
                break;
            }
            index++;
        }
        // Close the registry key
        RegCloseKey(hKey);
    }

}

void AppendFonts(HWND list, std::vector<FontST>* fonts){
    size_t howmanyfonts = (*fonts).size();
    for(int i=0; i<howmanyfonts; i++) {
        std::string n = (*fonts)[i].fontNAME + "  [" + (*fonts)[i].fontFILE + "]";
        int pos = (int)SendMessage(list, LB_ADDSTRING, 0, (LPARAM)n.c_str());
        
        SendMessage(list, LB_SETITEMDATA, pos, (LPARAM)i);
    }
}

HWND list;
HWND previewtxt;
std::vector<FontST> fonts;
HFONT hPreviewTextFont;
static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    switch (msg) {

    case WM_INITDIALOG: {
        fonts.clear();

        list = GetDlgItem(hwnd, FontDialogListComponent);
        previewtxt = GetDlgItem(hwnd, PreviewTXT);

        LoadupFonts(&fonts);
        AppendFonts(list, &fonts);

        
        return FALSE;
    }
    case WM_HSCROLL: {
        return TRUE;
    }
    case WM_COMMAND: {

        switch (wparam) {
            case IDOK: {
                EndDialog(hwnd, IDOK);
                break;
            }
            case IDCANCEL: {
                EndDialog(hwnd, IDCANCEL);
            }
        }
        if (HIWORD(wparam) == LBN_SELCHANGE) {
            int selectedPos = SendMessage(list, LB_GETCURSEL, 0, 0);
            if (selectedPos != LB_ERR) // Ensure kindness by checking for valid selection
            {
                LPARAM customData = SendMessage(list, LB_GETITEMDATA, selectedPos, 0);

                char itemtxt[256];
                SendMessage(list, LB_GETTEXT, customData, (LPARAM)itemtxt);
                selectedFont = fonts[customData].fontFILE;

                if (hPreviewTextFont) {
                    DeleteObject(hPreviewTextFont);
                }
                const char* ft_cstr = fonts[customData].fontNAME.c_str();
                hPreviewTextFont = CreateFont(
                    17, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    ft_cstr
                );
                if (hPreviewTextFont) {
                    SendMessage(previewtxt, WM_SETFONT, (WPARAM)hPreviewTextFont, TRUE);
                }
            }
        }
        break;
    }
    case WM_CLOSE: {
        EndDialog(hwnd, IDCANCEL);
    }
    default: {
        return FALSE;
    }
    }
    return TRUE;
}