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

## Controls

- `Ctrl+Z` undo
- `Ctrl+Y` redo
- `Ctrl+S` save to `drawing.cdraw`
- `Ctrl+O` load from `drawing.cdraw`
- `Ctrl+N` new canvas
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
