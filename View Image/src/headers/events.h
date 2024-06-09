#pragma once
#include "globalvar.h"
#include "ops.h"
#include "rendering.h"
#include "imgload.h"
#include "leftrightlogic.h"
#include <string>
#include "dialogcontrol.h"


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


void createUndoStep(GlobalParams* m);


void TurnOnDraw(GlobalParams* m);
void TurnOffDraw(GlobalParams* m);