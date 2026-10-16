# Mural

Mural is a small wallpaper browser for Linux written in C.

It uses OpenGL for rendering and X11 for the desktop environment.

## Features

- Browse wallpapers in a thumbnail grid
- Variable thumbnail heights
- Smooth scrolling
- Background image loading
- OpenGL texture atlas
- Rounded thumbnails
- Configurable appearance
- Select a wallpaper with a click
- Restore the last selected wallpaper
- TOML configuration

## Requirements

- Linux
- X11
- OpenGL
- GLFW
- GLEW
- feh

## Configuration

Mural uses `mural.toml`.

```toml
[general]
scroll_speed = 30.0
font_file = "./font.otf"
font_size = 20.0

[layout]
thumbnail_width  = 120.0
thumbnail_radius = 10.0
margin           = 10.0
inner_gap        = 7.0
border_thickness = 2.0

[color]
background      = 0x2f3e46ff
border_select   = 0x457b9dff
border_unselect = 0xcad2c5ff
font_color      = 0xcad2c5ff

[wallpapers]
paths = [
  "/home/user/Wallpapers"
]
