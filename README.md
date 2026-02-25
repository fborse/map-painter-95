# Map Painter 95

## Background

In a previous world of limited working memory, video games organised the graphics of their worlds as
arrangements of **repeating tiles**, which would be loaded into memory and displaying the game's
graphics would thus consist of blitting for each coordinate its corresponding tile.
The concept of **tiling** is thus the process of assigning a tile to every coordinate, thus creating
a **map**.

In the current world, working memory is not a realistic problem anymore, but the concept of tiling
has subsisted : while most entities in a world are unique, most share common features, which can be
factorised well enough by combining tiles, just as in the days of old.

*Map Painter 95* comes thus at a crossroad for 2D video games : creating game maps with a limited
set of tiles often creates an inflexibility in tooling regarding tile editing/creation, while
generating a new map from empty bitmaps is equally cumbersome, as repeated elements must be
copy/pasted manually.

*Map Painter 95* provides instead an approach where one can interactively assign tiles to every
coordinate on a map (*Map Editor* component) while also editing the tiles directly on a rendered
view of the map (*Map Painter* component).

Unlike other similar tools, *Map Painter 95* also allows to organise the tiles as a ready-to-export
**tileset**, usable by a video game for the rendering of their graphics.
Similarly, instead of storing tile indexes -- which always carry the risk of out-of-bounds -- or
even actual pixel information, a map could then focus on providing a grid of coordinates on the
exported tileset bitmap.

The ultimate purpose of *Map Painter 95* is to provide easily-importable and lightweight data for
other projects, such as 2D game engines or even textures for 3D contexts, more than serve as an
all-in-one solution.
The editor components themselves are a fusion of what one would expect from *RPG Maker* and general
purpose drawing tools.

## Usage

### Obtaining an executable

*Map Painter 95* is written in *C++*, depends on *Qt6*, and uses `qmake`.
This means that in order to obtain a runnable executable, one has to first run qmake :
```
qmake map-painter-95.pro
```

Note that many Linux distributions package Qt6 differently from Qt5, and that it may be necessary to
call `qmake6` explicitly.

This generates a *Makefile*, which contains the usual compilation rules.
A typical command would be (replace 9 with the number of CPUs available on your machine plus 1) :
```
make -j9
```

This should create an executable `map-painter-95`.
