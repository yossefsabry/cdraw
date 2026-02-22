# cdraw

`cdraw` is a lightweight vector drawing app built in C with [raylib](https://www.raylib.com/).

![cdraw screenshot](screen.png)

## Features

- Pen, rectangle, circle, arrow/line, eraser, and selection tools
- Zoom and pan (infinite-canvas style camera)
- Grid toggle
- Color palette + picker and stroke thickness control
- Save/load a simple `.cdraw` file format

## Build

### Dependencies

- C compiler + `make`
- `raylib` (development headers + library)
- `rsvg-convert` (used by `make ui-icons` to convert `public/*.svg` into `assets/ui_icons/*.png`)

### Compile

```sh
make
```

Debug build:

```sh
make DEBUG=1
```

Clean:

```sh
make clean
```

## Run

```sh
./cdraw
```

## AI (Local / Ollama)

`cdraw` can use any OpenAI-compatible local model server.

1. Start Ollama:

```sh
ollama serve
```

2. In the app, open `Menu -> AI -> Settings` and set:

- Provider: `Local`
- Model: any installed Ollama model (e.g. `llama3`)
- Base URL: `http://localhost:11434/v1`

Optional environment variables:

- `CDRAW_AI_PROVIDER=local`
- `CDRAW_AI_MODEL=llama3`
- `CDRAW_AI_BASE_URL=http://localhost:11434/v1`

## AI Backend (C)

`cdraw` starts a local background server to handle AI requests and math
calculation. The server lives in `backend_ai/` and listens on `127.0.0.1:8900`.

Build:

```sh
make backend-ai
```

Gemini key (used by the backend):

```sh
export GEMINI_API_KEY=your_key_here
```

Optional override:

```sh
export CDRAW_AI_BACKEND_URL=http://127.0.0.1:8900
```

## Controls

- `Ctrl+Z` undo
- `Ctrl+Y` redo
- `Ctrl+S` save to `drawing.cdraw`
- `Ctrl+O` load from `drawing.cdraw`
- `Ctrl+N` new canvas
- Tool shortcuts: `P` pen, `B` rectangle, `C` circle, `A` arrow/line, `E` eraser, `M` move/select
- `G` toggle grid
- `F` toggle fullscreen
- `Esc` close menus/pickers
- Mouse wheel: zoom
- Right mouse drag: pan
- `Space` + left drag: pan
- `Ctrl+Shift` + mouse wheel: adjust thickness
- Selection tool: click a stroke to select, drag to move, `Delete` to remove

## File format

Saved drawings use a plain-text format with a `CDRAW1` header, then a list of strokes and their points. See `src/canvas_io.c` for the exact format.

## License

MIT — see `LICENSE`.
