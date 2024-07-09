#pragma once
#include "globalvar.hpp"
#include "ops.hpp"
#include "rendering.hpp"
#include "imgload.hpp"
#include "leftrightlogic.hpp"
#include <string>
#include "resizedialog.hpp"

#include "../effectdialog/headers/brightnesscontrast.h"
#include "../effectdialog/headers/gaussian.h"


bool Initialization(GlobalParams* m, int argc, LPWSTR* argv);
bool ToolbarMouseDown(GlobalParams* m);
void MouseDown(GlobalParams* m);
void MouseMove(GlobalParams* m);
void KeyDown(GlobalParams* m, WPARAM wparam, LPARAM lparam);
void MouseUp(GlobalParams* m);
void RightDown(GlobalParams* m);
void RightUp(GlobalParams* m);
void Size(GlobalParams* m);
void MouseWheel(GlobalParams* m, WPARAM wparam, LPARAM lparam);

void ConvertToPremultipliedAlpha(uint32_t* imageData, int width, int height);
void ToggleFullscreen(GlobalParams* m);


void createUndoStep(GlobalParams* m, bool async);
void ShowMyInformation(GlobalParams* m);


void TurnOnDraw(GlobalParams* m);
void TurnOffDraw(GlobalParams* m);