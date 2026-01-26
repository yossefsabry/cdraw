#ifndef GUI_H
#define GUI_H

#include "canvas.h"
#include "raylib.h"

typedef struct FileDialogEntry {
  char path[256];
  char name[128];
  bool isDir;
  bool isUp;
} FileDialogEntry;

// ToolType is defined in canvas.h

typedef struct {
  Rectangle src;
} GuiIcon;

typedef struct {
  Texture2D atlas;

  GuiIcon menu;
  GuiIcon add;
  GuiIcon openFile;
  GuiIcon saveFile;
  GuiIcon undo;
  GuiIcon redo;

  GuiIcon pen;
  GuiIcon rect;
  GuiIcon circle;
  GuiIcon line;
  GuiIcon eraser;
  GuiIcon select;
  GuiIcon pan;

  GuiIcon grid;
  GuiIcon fullscreen;
  GuiIcon resetView;

  GuiIcon colorPicker;

  GuiIcon darkMode;
  GuiIcon lightMode;

  GuiIcon windowMinimize;
  GuiIcon windowToggleSize;
  GuiIcon windowClose;
} GuiIcons;

typedef struct {
  int activeTool;
  Color currentColor;
  float currentThickness;
  Rectangle toolbarRect;

  // Slider State
  bool isDraggingSize;

  // Rulers
  bool showRulers;
  Rectangle rulerTopRect;
  Rectangle rulerLeftRect;

  // Welcome
  bool showWelcome;
  bool hasSeenWelcome;

  // Palette State
  Rectangle paletteRect;
  Rectangle paletteButtonRect;

  // Color Picker State
  bool showColorPicker;
  Rectangle colorPickerRect;
  Rectangle colorButtonRect;
  float hueValue;
  float satValue;
  float valValue;

  // UI State
  bool darkMode;
  bool showMenu;
  Rectangle menuRect;
  Rectangle menuButtonRect;
  bool requestExit;
  bool isTyping;

  char currentFile[256];
  bool hasFilePath;
  char lastDir[256];
  bool showFileDialog;
  bool fileDialogIsSave;
  char fileDialogDir[256];
  char fileDialogName[256];
  FileDialogEntry *fileDialogEntries;
  int fileDialogEntryCount;
  int fileDialogSelected;
  int fileDialogScroll;
  double fileDialogLastClickTime;
  int fileDialogLastClickIndex;
  char toast[128];
  double toastUntil;

  GuiIcons icons;
  Font uiFont;
  bool ownsUiFont;
} GuiState;

void InitGui(GuiState *gui);
void UnloadGui(GuiState *gui);
void DrawGui(GuiState *gui, Canvas *canvas);
void UpdateGui(GuiState *gui, Canvas *canvas);
bool IsMouseOverGui(GuiState *gui);
void UpdateCursor(GuiState *gui, const Canvas *canvas, bool mouseOverGui);
void DrawCursorOverlay(GuiState *gui, const Canvas *canvas, bool mouseOverGui);

#endif // GUI_H
