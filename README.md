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

TBD

## Program architecture

### MainWindow and data sharing model

The main window contains the editor widgets. Additionally, it also owns pointers to the data (and an
undo stack), which are shared to the editor widgets on construction of MainWindow.
All pointers here are `QSharedPointer<T>`.
It is always assumed that changes on the data by one editor widget could impact the rendering of all
other widgets, and thus one would need to update their view.

Additionally, in order for undo/redo operations to affect the shared data, the pointers are stored
by any undo command as `QWeakPointer<T>`.

### Tileset and map data models

TBD
