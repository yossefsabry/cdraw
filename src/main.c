#include "canvas.h"
#include "gui.h"
#include "prefs.h"
#include "raylib.h"

int main(void) {
  const int screenWidth = 1000;
  const int screenHeight = 800;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(screenWidth, screenHeight, "cdraw - Vector Drawing");
  SetExitKey(KEY_NULL);

  Canvas canvas;
  InitCanvas(&canvas, screenWidth, screenHeight);

  GuiState gui;
  InitGui(&gui);

  AppPrefs prefs = PrefsDefaults();
  (void)PrefsLoad(&prefs);
  gui.darkMode = prefs.darkMode;
  canvas.showGrid = prefs.showGrid;
  gui.hasSeenWelcome = prefs.hasSeenWelcome;
  gui.showWelcome = !prefs.hasSeenWelcome;
  if (gui.showWelcome)
    gui.hasSeenWelcome = true;

  SetTargetFPS(60);

  while (!WindowShouldClose() && !gui.requestExit) {
    // Update
    bool mouseOverGui = IsMouseOverGui(&gui);

    UpdateGui(&gui, &canvas);
    UpdateCanvasState(&canvas, mouseOverGui, gui.activeTool);
    UpdateCursor(&gui, &canvas, mouseOverGui);

    // Draw
    BeginDrawing();
    DrawCanvas(&canvas);
    DrawGui(&gui, &canvas);
    DrawCursorOverlay(&gui, &canvas, mouseOverGui);
    EndDrawing();
  }

  AppPrefs finalPrefs = PrefsDefaults();
  finalPrefs.darkMode = gui.darkMode;
  finalPrefs.showGrid = canvas.showGrid;
  finalPrefs.hasSeenWelcome = gui.hasSeenWelcome;
  (void)PrefsSave(&finalPrefs);

  UnloadGui(&gui);
  FreeCanvas(&canvas);
  ShowCursor();
  SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  CloseWindow();

  return 0;
}
