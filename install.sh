#!/usr/bin/env bash
set -euo pipefail

APP_NAME="Cdraw"
APP_ID="cdraw"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_SRC="${ROOT_DIR}/cdraw"
ICON_SRC="${ROOT_DIR}/assets/cdraw.svg"
ASSETS_SRC="${ROOT_DIR}/assets"
BACKEND_SRC="${ROOT_DIR}/backend_ai/backend_ai"

INSTALL_BIN="${HOME}/.local/bin/${APP_ID}"
DESKTOP_DIR="${HOME}/.local/share/applications"
ICON_DIR="${HOME}/.local/share/icons/hicolor/scalable/apps"
DATA_DIR="${HOME}/.local/share/${APP_ID}"
ASSETS_DEST="${DATA_DIR}/assets"
BACKEND_DEST_DIR="${DATA_DIR}/backend_ai"
BACKEND_DEST="${BACKEND_DEST_DIR}/backend_ai"
DESKTOP_FILE="${DESKTOP_DIR}/${APP_ID}.desktop"
ICON_DEST="${ICON_DIR}/${APP_ID}.svg"

mkdir -p "${HOME}/.local/bin" "${DESKTOP_DIR}" "${ICON_DIR}" \
  "${DATA_DIR}" "${BACKEND_DEST_DIR}"

if [[ ! -f "${BIN_SRC}" ]]; then
  echo "Binary not found at ${BIN_SRC}. Build it before installing."
  exit 1
fi

rm -f "${DESKTOP_FILE}" "${ICON_DEST}"

cp -f "${BIN_SRC}" "${INSTALL_BIN}"
chmod +x "${INSTALL_BIN}"

if [[ -d "${ASSETS_SRC}" ]]; then
  rm -rf "${ASSETS_DEST}"
  cp -R "${ASSETS_SRC}" "${ASSETS_DEST}"
else
  echo "Warning: assets not found at ${ASSETS_SRC}. UI icons may be missing."
fi

if [[ -f "${BACKEND_SRC}" ]]; then
  cp -f "${BACKEND_SRC}" "${BACKEND_DEST}"
  chmod +x "${BACKEND_DEST}"
else
  echo "Warning: backend binary not found at ${BACKEND_SRC}."
fi

if [[ -f "${ICON_SRC}" ]]; then
  cp -f "${ICON_SRC}" "${ICON_DEST}"
fi

cat > "${DESKTOP_FILE}" <<EOF
[Desktop Entry]
Type=Application
Name=${APP_NAME}
Exec=env CDRAW_ROOT=${DATA_DIR} ${INSTALL_BIN}
Icon=${ICON_DEST}
Terminal=false
Categories=Graphics;
StartupNotify=true
EOF

if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "${DESKTOP_DIR}" >/dev/null 2>&1 || true
fi

echo "Installed ${APP_NAME}. You can launch it from rofi."
