#ifndef GUI_H
#define GUI_H

#include "canvas.h"
#include "raylib.h"


// ToolType is defined in canvas.h

typedef struct {
  Rectangle src;
} GuiIcon;

typedef struct {
  Canvas canvas;
  char path[256];
  bool hasPath;
} Document;

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
  bool showExportMenu;
  Rectangle exportMenuRect;
  bool showHelp;
  Rectangle helpRect;
  Rectangle helpButtonRect;
  bool showAiMenu;
  Rectangle aiMenuRect;
  bool showAiSettings;
  Rectangle aiSettingsRect;
  int aiInputFocus;
  bool aiKeyReveal;
  int aiProvider;
  char aiModel[64];
  char aiKey[128];
  char aiBase[128];
  char aiStatus[128];
  bool showAiPanel;
  Rectangle aiRect;
  bool aiBusy;
  char aiText[2048];
  char aiError[128];
  bool requestExit;
  bool isTyping;

  Document *documents;
  int documentCount;
  int documentCapacity;
  int activeDocument;
  char lastFileName[128];
  char lastDir[256];
  char toast[128];
  double toastUntil;

  char tooltip[128];
  Rectangle tooltipAnchor;
  bool showTooltip;

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

void GuiDocumentsInit(GuiState *gui, int screenWidth, int screenHeight,
                      bool showGrid);
void GuiDocumentsFree(GuiState *gui);
Document *GuiGetActiveDocument(GuiState *gui);
Canvas *GuiGetActiveCanvas(GuiState *gui);

#endif // GUI_H
