# CellUI

Native C++26 terminal UI framework inspired by Ink/React-style CLIs.
Goal: build a declarative, component-based terminal renderer with polished Unicode-cell graphics, not a GUI toolkit and not an AI-agent app.

## Core Objective

Implement a terminal-native UI framework where every visual element renders into a styled cell buffer:

* text
* panels
* floating modals
* buttons
* lists
* status bars
* plots
* pseudo-3D visualizations

The framework must support the visual style demonstrated in the Python prototype: Unicode box drawing, block/shade glyphs, ANSI true color, clean dark theme, and animated dashboard-style layouts.

## Non-Goals

Do not implement AI-agent workflows.
Do not use browser rendering, HTML, CSS, Electron, ncurses UI widgets, or terminal image protocols for the MVP.
Do not start with raster graphics. The MVP must be cell-native.

## Target Language

Use **C++26 as the primary implementation language and standard target**.

The project should assume a C++26-capable compiler/toolchain and should actively use C++26 where it improves the design.

Required baseline:

* CMake project
* `CMAKE_CXX_STANDARD 26`
* modern C++26 idioms throughout the core library
* no dependency on C++23 compatibility as a design constraint
* no heavy external dependencies for MVP

Prefer C++26 facilities for:

* reflection-assisted component metadata where practical
* contracts for layout/rendering invariants
* `std::inplace_vector` for small fixed-capacity render/layout batches
* `std::linalg` for 2D/3D transforms where useful
* `std::execution` for future event-stream and async rendering architecture

The first implementation may provide small compatibility shims only when a C++26 library facility is not yet fully exposed by a specific compiler, but the architecture should remain explicitly C++26-first.

## Architecture

Project layout:

```text
cellui/
  CMakeLists.txt
  include/cellui/
    app.hpp
    color.hpp
    style.hpp
    cell.hpp
    screen_buffer.hpp
    renderer.hpp
    terminal.hpp
    event.hpp
    layout.hpp
    component.hpp
    widgets.hpp
    plot.hpp
  src/
    app.cpp
    ansi.cpp
    screen_buffer.cpp
    diff_renderer.cpp
    terminal.cpp
    keyboard.cpp
    layout.cpp
    widgets.cpp
    plot.cpp
  examples/
    dashboard.cpp
    plot_gallery.cpp
    theme_selector.cpp
  tests/
    screen_buffer_tests.cpp
    layout_tests.cpp
    renderer_tests.cpp
```

## Rendering Model

Everything renders into a virtual terminal framebuffer.

Core types:

```cpp
struct Color {
    uint8_t r, g, b;
};

struct Style {
    Color fg;
    Color bg;
    bool bold = false;
    bool dim = false;
    bool italic = false;
    bool underline = false;
};

struct Cell {
    std::string grapheme;
    Style style;
};

class ScreenBuffer {
public:
    ScreenBuffer(int width, int height);

    void put(int x, int y, std::string_view glyph, Style style);
    void text(int x, int y, std::string_view text, Style style);
    void fill(Rect rect, std::string_view glyph, Style style);
    void box(Rect rect, BorderStyle border, Style style);
};
```

Use ANSI true-color escape sequences for output.
Use alternate screen mode for fullscreen examples.
Implement a diff renderer that compares previous and current buffers and emits minimal terminal updates.

## Layout Model

Implement a small Flexbox-like layout system.

Required primitives:

* `Rect`
* `Size`
* `Constraints`
* `Padding`
* `HStack`
* `VStack`
* `Stack`
* `Spacer`
* `Fixed`
* `Flex`
* `Box`
* `ScrollArea`

`Stack` is used for overlays, modals, dropdowns, and floating panels. Later children render over earlier children.

## Component Model

Use a simple retained/declarative component model first.

Each component should support:

```cpp
class Component {
public:
    virtual Size measure(const Constraints&) = 0;
    virtual void layout(Rect bounds) = 0;
    virtual void paint(ScreenBuffer&) = 0;
    virtual bool event(const Event&) = 0;
    virtual ~Component() = default;
};
```

Avoid implementing full React reconciliation at first. Prioritize a clean, usable component API.

Example target API:

```cpp
auto app = Dashboard{
    Header{"CellUI Plot Lab"},
    HStack{
        Controls{state.selected_signal, state.paused},
        VStack{
            MetricStrip{state.metrics},
            Plot2D{state.signal},
            SphereMesh3D{}
        }
    },
    StatusBar{"q quit · ↑/↓ select · Space pause"}
};
```

## Terminal Input

Implement raw-mode keyboard handling.

Required events:

* arrow keys
* Enter
* Escape
* Tab
* Backspace
* printable characters
* terminal resize
* timer tick

Mouse support is optional after MVP.

Event model:

```cpp
using Event = std::variant<KeyEvent, MouseEvent, ResizeEvent, TickEvent>;
```

Implement basic focus management for selectable widgets.

## Widgets

MVP widgets:

* `Text`
* `Box`
* `Button`
* `List`
* `Select`
* `StatusBar`
* `Modal`
* `MetricCard`
* `Plot2D`
* `SphereMesh3D`

Later widgets:

* `Tabs`
* `Table`
* `TextInput`
* `Tooltip`
* `CommandPalette`
* `ProgressBar`
* `ScrollView`

## Plot System

The plotting subsystem must be cell-native.

Use these glyphs:

```text
█ ▓ ▒ ░ ▀ ▄ ─ │ ╱ ╲ ● ·
```

Implement:

* clean axes
* subtle grid
* line plot using half-block glyphs
* area fill using shade glyphs
* bar plot using full blocks
* dense field plot using shade density
* gradient coloring with ANSI true color

Plot API target:

```cpp
Plot2D{
    .title = "Signal response",
    .x_label = "time",
    .y_label = "amplitude",
    .series = {
        LineSeries{
            .data = samples,
            .style = LineStyle::HalfBlock,
            .fill = FillStyle::Shade
        }
    }
};
```

## 3D Demo

Implement a `SphereMesh3D` component.

Visual target: a filled amber low-poly sphere with triangular wire mesh, similar to a Matplotlib 3D sphere mesh, but rendered using terminal cells.

Technique:

* compute projected sphere position
* fill sphere body with `█`, `▓`, `▒`
* use Lambert-like shading for amber body colors
* draw triangular mesh using `─`, `│`, `╱`, `╲`
* draw subtle background reference grid
* keep it static for MVP

Do not use PNGs or terminal graphics protocols.

## Theme

Create a semantic theme structure:

```cpp
struct Theme {
    Color bg;
    Color panel;
    Color panel2;
    Color text;
    Color muted;
    Color dim;
    Color border;
    Color accent;
    Color cyan;
    Color pink;
    Color yellow;
    Color green;
    Color red;
};
```

Use semantic colors throughout the codebase. Avoid hardcoded colors inside components.

## MVP Demo

Create executable:

```bash
cellui-plot-lab
```

Expected UI:

```text
╭─ Dashboard ─────────────────────────────────────────────╮
│ Cell-native rendering: no images, no browser canvas.    │
│                                                        │
│ ╭─ Controls ─────╮  ╭─ Metrics ─────────────────────╮  │
│ │ ● Damped wave  │  │ samples 512  latency 9.2ms    │  │
│ │   Bars         │  ╰───────────────────────────────╯  │
│ │   Dense field  │  ╭─ Signal response ─────────────╮  │
│ ╰────────────────╯  │ block/shade line plot          │  │
│                     ╰────────────────────────────────╯  │
│                     ╭─ Static 3D sphere mesh ────────╮  │
│                     │ filled low-poly sphere          │  │
│                     ╰────────────────────────────────╯  │
╰────────────────────────────────────────────────────────╯
```

Required controls:

```text
q / Esc    quit
↑ / ↓      select signal
← / →      adjust phase
Space      pause/resume
```

## Implementation Order

1. CMake project scaffold
2. `Color`, `Style`, `Cell`, `Rect`, `ScreenBuffer`
3. ANSI true-color renderer
4. alternate screen and raw terminal mode
5. full-frame rendering
6. diff rendering
7. layout primitives
8. keyboard events
9. basic widgets
10. `Plot2D`
11. `SphereMesh3D`
12. dashboard example
13. tests and cleanup

## Quality Bar

The demo must look polished in a modern terminal.

Prioritize:

* clean spacing
* subtle borders
* dark theme
* consistent colors
* no noisy glyph clutter
* graceful resize behavior
* readable plots
* no flickering
* simple public API

The final result should feel like a native C++ version of an Ink-style terminal UI framework, with high-quality cell-native plotting as the showcase.

