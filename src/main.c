#include "canvas.h"
#include "gui.h"
#include "raylib.h"

int main(void) {
  const int screenWidth = 1000;
  const int screenHeight = 800;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(screenWidth, screenHeight, "cdraw - Vector Drawing");

  Canvas canvas;
  InitCanvas(&canvas, screenWidth, screenHeight);

  GuiState gui;
  InitGui(&gui);

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    // Update
    bool mouseOverGui = IsMouseOverGui(&gui);

    UpdateGui(&gui, &canvas);
    UpdateCanvasState(&canvas, mouseOverGui, gui.activeTool);

    // Draw
    BeginDrawing();
    DrawCanvas(&canvas);
    DrawGui(&gui, &canvas);
    EndDrawing();
  }

  FreeCanvas(&canvas);
  CloseWindow();

  return 0;
}
