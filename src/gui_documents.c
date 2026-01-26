#include "gui_internal.h"
#include <stdlib.h>
#include <string.h>

static Document *GuiAddDocumentInternal(GuiState *gui, int screenWidth,
                                        int screenHeight, bool showGrid,
                                        bool makeActive) {
  if (gui->documentCount >= gui->documentCapacity) {
    int newCap = (gui->documentCapacity == 0) ? 4 : gui->documentCapacity * 2;
    Document *next =
        (Document *)realloc(gui->documents, sizeof(Document) * (size_t)newCap);
    if (!next)
      return NULL;
    gui->documents = next;
    gui->documentCapacity = newCap;
  }

  Document *doc = &gui->documents[gui->documentCount++];
  memset(doc, 0, sizeof(Document));
  InitCanvas(&doc->canvas, screenWidth, screenHeight);
  doc->canvas.showGrid = showGrid;
  doc->hasPath = false;
  doc->path[0] = '\0';

  if (makeActive)
    gui->activeDocument = gui->documentCount - 1;

  return doc;
}

void GuiDocumentsInit(GuiState *gui, int screenWidth, int screenHeight,
                      bool showGrid) {
  gui->documents = NULL;
  gui->documentCount = 0;
  gui->documentCapacity = 0;
  gui->activeDocument = -1;
  GuiAddDocumentInternal(gui, screenWidth, screenHeight, showGrid, true);
}

void GuiDocumentsFree(GuiState *gui) {
  for (int i = 0; i < gui->documentCount; i++)
    FreeCanvas(&gui->documents[i].canvas);
  free(gui->documents);
  gui->documents = NULL;
  gui->documentCount = 0;
  gui->documentCapacity = 0;
  gui->activeDocument = -1;
}

Document *GuiGetActiveDocument(GuiState *gui) {
  if (gui->documentCount <= 0)
    return NULL;
  if (gui->activeDocument < 0 || gui->activeDocument >= gui->documentCount)
    gui->activeDocument = 0;
  return &gui->documents[gui->activeDocument];
}

Canvas *GuiGetActiveCanvas(GuiState *gui) {
  Document *doc = GuiGetActiveDocument(gui);
  if (!doc)
    return NULL;
  return &doc->canvas;
}

Document *GuiAddDocument(GuiState *gui, int screenWidth, int screenHeight,
                         bool showGrid, bool makeActive) {
  return GuiAddDocumentInternal(gui, screenWidth, screenHeight, showGrid,
                                makeActive);
}

void GuiCloseDocument(GuiState *gui, int index, int screenWidth, int screenHeight,
                      bool showGrid) {
  if (index < 0 || index >= gui->documentCount)
    return;

  FreeCanvas(&gui->documents[index].canvas);
  for (int i = index; i < gui->documentCount - 1; i++)
    gui->documents[i] = gui->documents[i + 1];
  gui->documentCount--;

  if (gui->documentCount <= 0) {
    GuiAddDocumentInternal(gui, screenWidth, screenHeight, showGrid, true);
    return;
  }

  if (gui->activeDocument > index)
    gui->activeDocument--;
  else if (gui->activeDocument == index)
    gui->activeDocument = (index > 0) ? index - 1 : 0;
}
