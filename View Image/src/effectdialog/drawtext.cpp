#include "headers/drawtext.h"
#include "../../resource.h"
#include <Uxtheme.h>
#include <iostream>
#include <string>
#include "../headers/rendering.hpp"
//#include <dwmapi.h>
bool dontdo = false;

COLORREF boxColor = RGB(255, 0, 0); // Initial color red
HBRUSH hBrush = NULL;


static GlobalParams* m;

std::string text = "Cosine64";
std::string fontstring = "segoeui.ttf";
int locationX = 0;
int locationY = 0;
int textSize = 32;
int bevelAmount = 0;
int outlineAmount = 0;

uint32_t textColor = 0xFFFF80FF; // inverted
uint32_t outlineColor = 0xFF000000; // global so we can save it

// Function prototypes
static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

bool didinit = false;
int ShowDrawTextDialog(GlobalParams* m0) {
    didinit = false;
    dontdo = false;
    m = m0;
    if (m->drawmode) {
        m->drawmode = false;
    }

    // Create the main dialog

    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(DrawTextDialog), m->hwnd, (DLGPROC)DialogProc);

    return 0;
}

FT_Face f = nullptr;
static void ApplyEffectToBuffer(void* fromBuffer, void* toBuffer) {

    SwitchFont(f);

    memcpy(toBuffer, fromBuffer, m->imgwidth * m->imgheight * 4);
    PlaceString(m, textSize, text.c_str(), locationX, locationY, InvertCC(textColor, true), toBuffer, m->imgwidth, m->imgheight, fromBuffer);
    RedrawSurface(m);
}

static void ConfirmEffect(bool checked) {
    createUndoStep(m, false);
   // memcpy(m->imgdata, m->imagepreview, m->imgwidth * m->imgheight * 4);
    
    ApplyEffectToBuffer(m->imgdata, m->imgdata);
    if (!checked) {
        ApplyEffectToBuffer(m->imgoriginaldata, m->imgoriginaldata);
    }

    m->shouldSaveShutdown = true;
}


HWND adtdb;
HWND posXd;
HWND posYd;
HWND tss;
HWND AffectDrawingEraseBox;
HWND fnameid;

void UpdateLoadFont() {
    if (f) {
        FT_Done_Face(f);
    }
    f = LoadFont(m, fontstring);

    SetWindowText(fnameid, fontstring.c_str());

}

void InitDialogControls(HWND hwnd) {
    
    
    adtdb = GetDlgItem(hwnd, ActualDrawTextDialogBox);
    posXd = GetDlgItem(hwnd, posXs);
    posYd = GetDlgItem(hwnd, posYs);
    tss = GetDlgItem(hwnd, TextSizeSlider);
    fnameid = GetDlgItem(hwnd, FontNameID);


    AffectDrawingEraseBox = GetDlgItem(hwnd, AffectDrawingErase);
    SendMessage(AffectDrawingEraseBox, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);


    SetWindowText(adtdb, text.c_str());

    SendMessage(posXd, TBM_SETRANGE, TRUE, MAKELPARAM(0, m->imgwidth));
    SendMessage(posXd, TBM_SETPOS, TRUE, locationX);

    SendMessage(posYd, TBM_SETRANGE, TRUE, MAKELPARAM(0, m->imgheight));
    SendMessage(posYd, TBM_SETPOS, TRUE, locationY);

    SendMessage(tss, TBM_SETRANGE, TRUE, MAKELPARAM(1, 512));
    SendMessage(tss, TBM_SETPOS, TRUE, textSize);

    SetFocus(adtdb);
    SendMessage(adtdb, EM_SETSEL, 0, -1);

    UpdateLoadFont();
}

void UpdateImage() {
    if (dontdo) {
        return;
    }
    if (!didinit) {
        return;
    }
    char str[256];
    GetWindowText(adtdb, str, 256);

    float posx = (float)SendMessage(posXd, TBM_GETPOS, 0, 0);
    float posy = (float)SendMessage(posYd, TBM_GETPOS, 0, 0);
    float size = (float)SendMessage(tss, TBM_GETPOS, 0, 0);

    locationX = posx;
    locationY = posy;
    textSize = size;
    text = std::string(str);

    ApplyEffectToBuffer(m->imgdata, m->imagepreview);
}

void freeness() {
    m->isImagePreview = false;
    if (m->imagepreview) {
        FreeData(m->imagepreview);
    }
    
}
static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    switch (msg) {
        case WM_INITDIALOG: {


            RECT r;
            GetWindowRect(m->hwnd, &r);

            SetWindowPos(hwnd, 0, r.left+((m->CoordRight>m->width ? m->width : m->CoordRight)), r.top, 0, 0, SWP_NOSIZE);

            m->imagepreview = malloc(m->imgwidth * m->imgheight * 4);
            memcpy(m->imagepreview, m->imgdata, m->imgwidth * m->imgheight * 4);
            m->isImagePreview = true;

            InitDialogControls(hwnd);

            RedrawSurface(m);
            didinit = true;
            UpdateImage();
            return FALSE;
        }

        case WM_COMMAND: {
            UpdateImage();
            switch (LOWORD(wparam)) {
            case SelectFontButton: {
                std::string font = ShowFontDialog(m, hwnd);

                if (font != "Error") {
                    fontstring = font;
                    UpdateLoadFont();
                    UpdateImage();
                }
                break;
            }
            case ColorTextButton: {

                bool success = true;// alpha to 0 weird windows bug
                uint32_t c = change_alpha(PickColorFromDialog(m, change_alpha(textColor, 0), &success), 255);
                if (success) {
                    textColor = c;
                }
                SendMessage(hwnd, WM_CTLCOLORSTATIC, (WPARAM)GetDC(hwnd), (LPARAM)hwnd);
                
                HWND tb = GetDlgItem(hwnd, ColorTextBlock);

                RECT rect;
                GetClientRect(tb, &rect);
                InvalidateRect(tb, &rect, TRUE);
                MapWindowPoints(tb, hwnd, (POINT*)&rect, 2);
                RedrawWindow(hwnd, &rect, NULL, RDW_ERASE | RDW_INVALIDATE);

                UpdateImage();

                break;
            }
            case IDOK: {

                LRESULT result = SendMessage(AffectDrawingEraseBox, BM_GETCHECK, 0, 0);

                ConfirmEffect(result == BST_CHECKED);

                dontdo = true;
                freeness();
                EndDialog(hwnd, IDCANCEL);
                break;
            }
            case IDCANCEL: {
                dontdo = true;
                freeness();
                EndDialog(hwnd, IDCANCEL);
            }
            }
            break;
        }
        case WM_HSCROLL: {
            UpdateImage();
            return TRUE;
        }
    case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wparam;
            HWND hwndStatic = (HWND)lparam;

            if (GetDlgCtrlID(hwndStatic) == ColorTextBlock) {
                if (hBrush) {
                    DeleteObject(hBrush);
                }
                hBrush = CreateSolidBrush(change_alpha(textColor,0));
                SetBkColor(hdcStatic, change_alpha(textColor, 0));
                return (INT_PTR)hBrush;
            }
            else {
                return DefWindowProc(hwnd, msg, wparam, lparam);
            }
            break;
    }
    case WM_CLOSE: {
        dontdo = true;
            freeness();
            
            EndDialog(hwnd, IDCANCEL);
        }
    default: {
        return FALSE;
    }
    }
    return TRUE;
}