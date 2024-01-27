#include "headers/dialogcontrol.h"
#include "../resource.h"

GlobalParams* m;

// resize dialog
HWND hWidthEdit, hHeightEdit;

// Function prototypes
LRESULT CALLBACK ResizeDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

int ShowResizeDialog(GlobalParams* m0){
    m = m0;
    // Create the main dialog
    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(Resize), m->hwnd, ResizeDialogProc);

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

LRESULT CALLBACK ResizeDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (!m) {
        MessageBox(hwnd, "Once upon a time there was a little pointer called m. it travels all across our code, until one day, it got lost. we can't find m, so we don't know what to do here, therefore throwing an error together!", "Fatal Memory Transfer Error", MB_OK | MB_ICONERROR);
        return TRUE; // Do not proceed with resizing
    }
    switch (msg) {
    case WM_INITDIALOG: {
        // Initialize dialog controls and set default values
        hWidthEdit = GetDlgItem(hwnd, WidthPBox);
        hHeightEdit = GetDlgItem(hwnd, HeightPBox);

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

        return TRUE;
    }
    case WM_COMMAND: {
        switch (LOWORD(wparam)) {
        case WM_CHAR: {
            // Allow only numeric characters
            if (wparam < '0' || wparam > '9') {
                MessageBeep(MB_ICONERROR);
                return 0; // Ignore non-numeric characters
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
                return TRUE; // Do not proceed with resizing
            }

            // Convert text to integers
            int width = atoi(widthText);
            int height = atoi(heightText);

            if (width < 0 || height < 0) {
                MessageBox(hwnd, "You really though you could get away with having a negative width or height", "Error", MB_OK | MB_ICONERROR);
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
    default:
        return FALSE;
    }

    return TRUE;
}