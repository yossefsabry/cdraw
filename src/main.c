#include "app_paths.h"
#include "canvas.h"
#include "gui.h"
#include "prefs.h"
#include "raylib.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#if !defined(_WIN32)
static pid_t g_backend_pid = -1;

static void BackendSetEnv(void) {
  if (!getenv("CDRAW_AI_BACKEND_URL"))
    setenv("CDRAW_AI_BACKEND_URL", "http://127.0.0.1:8900", 0);
  if (!getenv("CDRAW_AI_USE_BACKEND"))
    setenv("CDRAW_AI_USE_BACKEND", "1", 0);
}

static bool BackendCanConnect(void) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return false;
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8900);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  bool ok = (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0);
  close(sock);
  return ok;
}

static void StartBackend(void) {
  char backendPath[PATH_MAX];
  if (!CdrawBackendPath(backendPath, sizeof(backendPath))) {
    fprintf(stderr, "Backend binary not found; skipping start\n");
    return;
  }

  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "Backend start failed: %s\n", strerror(errno));
    return;
  }
  if (pid == 0) {
    execl(backendPath, backendPath, NULL);
    fprintf(stderr, "Backend exec failed: %s\n", strerror(errno));
    _exit(1);
  }

  g_backend_pid = pid;
  for (int i = 0; i < 30; i++) {
    if (BackendCanConnect()) {
      fprintf(stderr, "Backend started on 127.0.0.1:8900\n");
      return;
    }
    usleep(100000);
  }
  fprintf(stderr, "Warning: backend not responding\n");
}

static void StopBackend(void) {
  if (g_backend_pid <= 0)
    return;
  kill(g_backend_pid, SIGTERM);
  int status = 0;
  waitpid(g_backend_pid, &status, 0);
  g_backend_pid = -1;
  fprintf(stderr, "Backend stopped\n");
}
#else
static void BackendSetEnv(void) {}
static void StartBackend(void) {}
static void StopBackend(void) {}
#endif

int main(void) {
  const int screenWidth = 1000;
  const int screenHeight = 800;

  BackendSetEnv();
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

  StartBackend();

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
  StopBackend();
  CloseWindow();

  return 0;
}
