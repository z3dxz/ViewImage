#include "headers/resizedialog.hpp"
#include "../resource.h"
#include <Uxtheme.h>
//#include <dwmapi.h>

GlobalParams* m;

// resize dialog
HWND hWidthEdit, hHeightEdit;
HWND Confirm, No, LW, LH;

// Function prototypes
LRESULT CALLBACK ResizeDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

int ShowResizeDialog(GlobalParams* m0){
    m = m0;
    // Create the main dialog
    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(Resize), m->hwnd, (DLGPROC)ResizeDialogProc);

    return 0;
}

// this should really be in ops.cpp but we aren't going to talk about it
bool IsNumeric(LPCTSTR str) { 
    for (int i = 0; str[i] != '\0'; ++i) {
        if (!iswdigit(str[i])) {
            return false;
        }
    }
    return true;
}

HBRUSH b;
HBRUSH be;
HBRUSH bz;
HBRUSH bu;

#pragma comment(lib, "Comctl32.lib")


HANDLE lockic = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(lockicon), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
HANDLE unlockic = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(unlockicon1), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);

bool iswlock = false;
bool ishlock = false;

int ogwidth;
int ogheight;

LRESULT CALLBACK TXTProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (msg)
    {
    case WM_CHAR:
    {
        // Allow only numeric characters (0-9) and control characters
        if (wParam < '0' || wParam > '9')
        {
            if (wParam != '\b' && wParam != '\r' && wParam != '\t' && wParam != '\x1A')
                return 0; // Block the input
        }
        break;
    }
    }

    // Call the original window procedure for default processing
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK ResizeDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    switch (msg) {

    case WM_INITDIALOG: {
        iswlock = false;
        ishlock = false;
        if (m->imgwidth < 1) {

            MessageBox(hwnd, "You need an image first", "Oops", MB_OK | MB_ICONERROR);

            INT Result = 1;
            EndDialog(hwnd, Result);
        }
        if (!m) {
            MessageBox(hwnd, "Once upon a time there was a little pointer called m. it travels all across our code, until one day, it got lost. we can't find m, so we don't know what to do here, therefore throwing an error together!", "Fatal Memory Transfer Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        // Initialize dialog controls and set default values
        hWidthEdit = GetDlgItem(hwnd, WidthPBox);
        hHeightEdit = GetDlgItem(hwnd, HeightPBox);
        Confirm = GetDlgItem(hwnd, ConfirmBBox);
        No = GetDlgItem(hwnd, CancelBBox);
        LW = GetDlgItem(hwnd, lockw);
        LH = GetDlgItem(hwnd, lockh);

        SetWindowSubclass(hWidthEdit, TXTProc, 0, 0);
        SetWindowSubclass(hHeightEdit, TXTProc, 0, 0);

        SendMessage(LW, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
        SendMessage(LH, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
        
        SetWindowTheme(Confirm, L"Darkmode_Explorer", NULL);
        SetWindowTheme(No, L"Darkmode_Explorer", NULL);

        SetWindowTheme(LW, L"Darkmode_Explorer", NULL);
        SetWindowTheme(LH, L"Darkmode_Explorer", NULL);

        int w1 = m->imgwidth;
        int h1 = m->imgheight;

        char strw[8];
        char strh[8];

        sprintf(strw, "%d", w1);
        sprintf(strh, "%d", h1);

        SetWindowText(hWidthEdit, strw);
        SetWindowText(hHeightEdit, strh);

        SetFocus(hWidthEdit);

        SendMessage(hWidthEdit, EM_SETSEL, 0, -1);

        BOOL enable = TRUE;
        //DwmSetWindowAttribute(hwnd, 20, &enable, sizeof(enable));
        if (!DwmDarken(hwnd)) {
            // windows XP
        }

        return FALSE;
    }
    case WM_CTLCOLORDLG: {

        if (bz) DeleteObject(bz);
        bz = CreateSolidBrush(RGB(15,15,15));
        return (LRESULT)bz;
    }
    
    case WM_CTLCOLORBTN: {
        if (bu) DeleteObject(bu);
        bu = CreateSolidBrush(RGB(0,0,0));
        return (LRESULT)bu;
    }
    case WM_CTLCOLORSTATIC: {

        DWORD CtrlID = GetDlgCtrlID((HWND)lparam);
        HDC hdcStatic = (HDC)wparam;

        if (b) DeleteObject(b);
        b = CreateSolidBrush(RGB(15,15,15));

        SetBkMode(hdcStatic, TRANSPARENT);

        // Set default text color
        SetTextColor(hdcStatic, RGB(200,200,200));


        return (LRESULT)b;
    }

    case WM_CTLCOLOREDIT: {
        DWORD CtrlID = GetDlgCtrlID((HWND)lparam);
        HDC hdcStatic = (HDC)wparam;

        if (be) DeleteObject(be);
        b = CreateSolidBrush(RGB(40,40,40));

        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, RGB(128,128,128));
        SetBkColor(hdcStatic, RGB(0, 255, 255));


       return (LRESULT)b;
    }
    case WM_COMMAND: {
        if ((LOWORD(wparam) == WidthPBox && HIWORD(wparam) == EN_CHANGE)&&ishlock) {
            const int nMaxCount = 64;

            char widthTxt[nMaxCount];
            GetWindowText(hWidthEdit, widthTxt, nMaxCount);

            int width = std::atoi(widthTxt);

            float asp = (float)ogwidth / (float)ogheight;

            int newheight = (float)width / asp;

            std::string newtxt = std::to_string(newheight);

            SetWindowText(hHeightEdit, newtxt.c_str());
        }

        if ((LOWORD(wparam) == HeightPBox && HIWORD(wparam) == EN_CHANGE) && iswlock) {
            const int nMaxCount = 64;

            char heightTxt[nMaxCount];
            GetWindowText(hHeightEdit, heightTxt, nMaxCount);

            int height = std::atoi(heightTxt);

            float asp = (float)ogwidth / (float)ogheight;

            int newwidth = (float)height * asp;

            std::string newtxt = std::to_string(newwidth);

            SetWindowText(hWidthEdit, newtxt.c_str());
        }

        switch (LOWORD(wparam)) {
        case lockw: {
            if (iswlock) {
                // undo
                iswlock = false;
                SendMessage(LW, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                SendMessage(LH, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                EnableWindow(hWidthEdit, TRUE);
                EnableWindow(hHeightEdit, TRUE);
            }
            else {
                iswlock = true;
                ishlock = false;
                SendMessage(LW, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)lockic);
                SendMessage(LH, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                EnableWindow(hWidthEdit, FALSE);
                EnableWindow(hHeightEdit, TRUE);

                char heightTxt[64];
                GetWindowText(hHeightEdit, heightTxt, 64);
                ogheight = std::atoi(heightTxt);
                char widthTxt[64];
                GetWindowText(hWidthEdit, widthTxt, 64);
                ogwidth = std::atoi(widthTxt);
            }
            break;
        }
        case lockh: {
            if (ishlock) {
                // undo
                ishlock = false;
                SendMessage(LW, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                SendMessage(LH, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                EnableWindow(hWidthEdit, TRUE);
                EnableWindow(hHeightEdit, TRUE);
            }
            else {
                iswlock = false;
                ishlock = true;
                SendMessage(LW, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)unlockic);
                SendMessage(LH, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)lockic);
                EnableWindow(hWidthEdit, TRUE);
                EnableWindow(hHeightEdit, FALSE);

                char heightTxt[64];
                GetWindowText(hHeightEdit, heightTxt, 64);
                ogheight = std::atoi(heightTxt);
                char widthTxt[64];
                GetWindowText(hWidthEdit, widthTxt, 64);
                ogwidth = std::atoi(widthTxt);
            }
            break;
        }
        case ConfirmBBox: {
            // Retrieve values from text boxes
            char widthText[16], heightText[16];
            GetWindowText(hWidthEdit, widthText, 16);
            GetWindowText(hHeightEdit, heightText, 16);

            if (!IsNumeric(widthText) || !IsNumeric(heightText)) {
                MessageBox(hwnd, "Width and height must be numeric!", "Error", MB_OK | MB_ICONERROR);
                SetFocus(hWidthEdit);
                SendMessage(hWidthEdit, EM_SETSEL, 0, -1);
                return TRUE; // Do not proceed with resizing
            }

            // Convert text to integers
            int width = atoi(widthText);
            int height = atoi(heightText);

            if (width > 16000 || height > 16000) {
                MessageBox(hwnd, "Too many pixels: Too high resolution", "Error", MB_OK | MB_ICONERROR);
                return TRUE; // Do not proceed with resizing
            }
            if (width < 1 || height < 1) {
                MessageBox(hwnd, "You need at least 1 pixel in each dimension", "Error", MB_OK | MB_ICONERROR);
                return TRUE; // Do not proceed with resizing
            }
            // Perform resizing logic with width and height
            ResizeImageToSize(m, width, height);

            // Close the dialog
            EndDialog(hwnd, IDOK);
            break;
        }
        case CancelBBox: {
            // Close the dialog without performing any action
            EndDialog(hwnd, IDCANCEL);
            break;
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