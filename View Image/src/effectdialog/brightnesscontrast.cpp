#include "headers/brightnesscontrast.h"
#include "../../resource.h"
#include <Uxtheme.h>
#include <dwmapi.h>

static GlobalParams* m;


HWND bslider;
HWND cslider;

// Function prototypes
static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

int ShowBrightnessContrastDialog(GlobalParams* m0) {
    m = m0;
    // Create the main dialog
    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(BrightnessContrast), m->hwnd, DialogProc);

    return 0;
}

int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static uint32_t AdjustBrightness(uint32_t color, int amount) {
    // Extract ARGB components
    int alpha = (color >> 24) & 0xFF;
    int red = (color >> 16) & 0xFF;
    int green = (color >> 8) & 0xFF;
    int blue = color & 0xFF;

    // Increase or decrease brightness for RGB components
    red = clamp(red + amount, 0, 255);
    green = clamp(green + amount, 0, 255);
    blue = clamp(blue + amount, 0, 255);

    // Recombine into 32-bit ARGB color
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

static uint32_t AdjustContrast(uint32_t color, float contrastFactor) {
    // Extract ARGB components
    int alpha = (color >> 24) & 0xFF;
    int red = (color >> 16) & 0xFF;
    int green = (color >> 8) & 0xFF;
    int blue = color & 0xFF;

    // Calculate contrast-adjusted RGB components
    float factor = (259.0f * (contrastFactor + 255.0f)) / (255.0f * (259.0f - contrastFactor));
    red = static_cast<int>(clamp(factor * (red - 128) + 128, 0.0f, 255.0f));
    green = static_cast<int>(clamp(factor * (green - 128) + 128, 0.0f, 255.0f));
    blue = static_cast<int>(clamp(factor * (blue - 128) + 128, 0.0f, 255.0f));

    // Recombine into 32-bit ARGB color
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

static void ApplyEffectToBuffer(int brightness, int contrast) {

    for (int y = 0; y < m->imgheight; y++) {
        for (int x = 0; x < m->imgwidth; x++) {
            uint32_t* from = GetMemoryLocation(m->imgdata, x, y, m->imgwidth, m->imgheight);
            uint32_t* to = GetMemoryLocation(m->imagepreview, x, y, m->imgwidth, m->imgheight);

            *to = AdjustBrightness(*from, brightness);
            *to = AdjustContrast(*to, contrast);
        }
    }

}

static void ConfirmEffect() {
    createUndoStep(m, false);
    memcpy(m->imgdata, m->imagepreview, m->imgwidth * m->imgheight * 4);
    m->shouldSaveShutdown = true;
}


static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    switch (msg) {

    case WM_INITDIALOG: {

        m->imagepreview = malloc(m->imgwidth * m->imgheight * 4);
        memcpy(m->imagepreview, m->imgdata, m->imgwidth * m->imgheight * 4);
        m->isImagePreview = true;

        BOOL enable = TRUE;
        DwmSetWindowAttribute(hwnd, 20, &enable, sizeof(enable));

        bslider = GetDlgItem(hwnd, SLIDERBRIGHTNESS);
        cslider = GetDlgItem(hwnd, SLIDERCONTRAST);

        SendMessage(bslider, TBM_SETRANGE, TRUE, MAKELPARAM(-100, 100));
        SendMessage(bslider, TBM_SETTICFREQ, 100, 0);
        SendMessage(bslider, TBM_SETPOS, TRUE, 0);

        SendMessage(cslider, TBM_SETRANGE, TRUE, MAKELPARAM(-100, 100));
        SendMessage(cslider, TBM_SETTICFREQ, 100, 0);
        SendMessage(cslider, TBM_SETPOS, TRUE, 0);

        RedrawSurface(m);
        return FALSE;
    }
    case WM_HSCROLL: {
        // Get the handle to the slider control

        // Get the current position of the slider
        int posb = (int)SendMessage(bslider, TBM_GETPOS, 0, 0);
        int brightness = posb;

        int posc = (int)SendMessage(cslider, TBM_GETPOS, 0, 0);
        int contrast = posc;

        ApplyEffectToBuffer(brightness, contrast);
        RedrawSurface(m);

        return TRUE;
    }
    case WM_KEYDOWN: {
        Beep(500, 500);
        RedrawSurface(m);
        break;
    }
   
    case WM_COMMAND: {
        switch (LOWORD(wparam)) {
        case IDOK: {
            // Confirm
            ConfirmEffect();

            m->isImagePreview = false;
            if (m->imagepreview) {
                free(m->imagepreview);
            }
            EndDialog(hwnd, IDCANCEL);
            break;
        }
        case IDCANCEL: {
            m->isImagePreview = false;
            if (m->imagepreview) {
                free(m->imagepreview);
            }
            EndDialog(hwnd, IDCANCEL);
        }
        }
        break;
    case WM_CLOSE: {
        m->isImagePreview = false;
        if (m->imagepreview) {
            free(m->imagepreview);
        }
        EndDialog(hwnd, IDCANCEL);
    }
    default:
        return FALSE;
    }
    }
    return TRUE;
}