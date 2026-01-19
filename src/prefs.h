#ifndef PREFS_H
#define PREFS_H

#include <stdbool.h>

typedef struct {
  bool darkMode;
  bool showGrid;
  bool hasSeenWelcome;
} AppPrefs;

AppPrefs PrefsDefaults(void);
bool PrefsLoad(AppPrefs *outPrefs);
bool PrefsSave(const AppPrefs *prefs);

#endif // PREFS_H

