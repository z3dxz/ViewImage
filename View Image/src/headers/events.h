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
void RightUp(GlobalParams* m);
void Size(GlobalParams* m);
void MouseWheel(GlobalParams* m, WPARAM wparam, LPARAM lparam);

void ToggleFullscreen(GlobalParams* m);