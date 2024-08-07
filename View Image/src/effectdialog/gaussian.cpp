#include "headers/gaussian.h"
#include "../../resource.h"
#include <Uxtheme.h>
//#include <dwmapi.h>

static GlobalParams* m;


HWND gslider;

// Function prototypes
static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

int ShowGaussianDialog(GlobalParams* m0) {
    m = m0;
    // Create the main dialog
    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(Gaussian), m->hwnd, (DLGPROC)DialogProc);

    return 0;
}

static void ApplyEffectToBuffer(float amount) {
    uint32_t* from = (uint32_t*)m->imgdata;
    uint32_t* to = (uint32_t*)m->imagepreview;
    //fast_gaussian_blur(from, to, m->imgwidth, m->imgheight, 4, 4.0f, 10, Border::kKernelCrop);
    //std::swap(m->imgdata, m->imagepreview);
    
    gaussian_blur_B(from, to, m->imgwidth, m->imgheight, amount, m->imgwidth, m->imgheight, 0, 0);
    
    
}

static void ConfirmEffect() {
    createUndoStep(m, false);
    memcpy(m->imgdata, m->imagepreview, m->imgwidth * m->imgheight * 4);
    m->shouldSaveShutdown = true;
}

LRESULT CALLBACK SliderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    switch (msg) {

    case WM_INITDIALOG: {

        m->imagepreview = malloc(m->imgwidth * m->imgheight * 4);
        memcpy(m->imagepreview, m->imgdata, m->imgwidth * m->imgheight * 4);
        m->isImagePreview = true;

        BOOL enable = TRUE;
        if (!DwmDarken(hwnd)) {
            // windows XP
        }
        //DwmSetWindowAttribute(hwnd, 20, &enable, sizeof(enable));

        gslider = GetDlgItem(hwnd, SLIDERGAUSSIAN);

        SendMessage(gslider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 40));
        SendMessage(gslider, TBM_SETPOS, TRUE, 0);

        SetWindowSubclass(gslider, SliderProc, 0, 0);

        RedrawSurface(m);
        return FALSE;
    }
    case WM_HSCROLL: {
        return TRUE;
    }
    case WM_COMMAND: {
        switch (LOWORD(wparam)) {
        case IDOK: {
            // Confirm
            ConfirmEffect();

            m->isImagePreview = false;
            if (m->imagepreview) {
                FreeData(m->imagepreview);
            }
            EndDialog(hwnd, IDCANCEL);
            break;
        }
        case IDCANCEL: {
            m->isImagePreview = false;
            if (m->imagepreview) {
                FreeData(m->imagepreview);
            }
            EndDialog(hwnd, IDCANCEL);
        }
        }
        break;
    case WM_CLOSE: {
        m->isImagePreview = false;
        if (m->imagepreview) {
            FreeData(m->imagepreview);
        }
        EndDialog(hwnd, IDCANCEL);
    }
    default:
        return FALSE;
    }
    }
    return TRUE;
}

LRESULT CALLBACK SliderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (msg)
    {
    case WM_LBUTTONUP:
    {
        float posg = (float)SendMessage(gslider, TBM_GETPOS, 0, 0);
        float amount = posg;

        ApplyEffectToBuffer(amount);
        RedrawSurface(m);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    default:
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
}