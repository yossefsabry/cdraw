#include "canvas.h"
#include "gui.h"
#include "prefs.h"
#include "raylib.h"
#include <stdio.h>

int main(void) {
  const int screenWidth = 1000;
  const int screenHeight = 800;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(screenWidth, screenHeight, "cdraw - Vector Drawing");
  SetExitKey(KEY_NULL);

  GuiState gui;
  InitGui(&gui);

  AppPrefs prefs = PrefsDefaults();
  (void)PrefsLoad(&prefs);
  gui.darkMode = prefs.darkMode;
  GuiDocumentsInit(&gui, screenWidth, screenHeight, prefs.showGrid);
  gui.hasSeenWelcome = prefs.hasSeenWelcome;
  gui.showWelcome = !prefs.hasSeenWelcome;
  if (gui.showWelcome)
    gui.hasSeenWelcome = true;

  SetTargetFPS(60);

  while (!WindowShouldClose() && !gui.requestExit) {
    // Update
    bool mouseOverGui = IsMouseOverGui(&gui);
    Canvas *canvas = GuiGetActiveCanvas(&gui);
    if (!canvas)
      continue;

    UpdateGui(&gui, canvas);
    UpdateCanvasState(canvas, mouseOverGui, gui.activeTool);
    UpdateCursor(&gui, canvas, mouseOverGui);

    // Draw
    BeginDrawing();
    DrawCanvas(canvas);
    DrawGui(&gui, canvas);
    DrawCursorOverlay(&gui, canvas, mouseOverGui);
    EndDrawing();
  }

  AppPrefs finalPrefs = PrefsDefaults();
  finalPrefs.darkMode = gui.darkMode;
  Document *doc = GuiGetActiveDocument(&gui);
  finalPrefs.showGrid = doc ? doc->canvas.showGrid : prefs.showGrid;
  finalPrefs.hasSeenWelcome = gui.hasSeenWelcome;
  (void)PrefsSave(&finalPrefs);

  GuiDocumentsFree(&gui);
  UnloadGui(&gui);
  ShowCursor();
  SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  CloseWindow();

  return 0;
}

void welcome() {
    printf("welcome");
}
