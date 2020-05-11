Screencast
==========

[![License](https://img.shields.io/badge/license-GPLv3.0-blue.svg)](https://www.gnu.org/licenses/gpl-3.0.html)
[![GitHub release](https://img.shields.io/github/release/lirios/screencast.svg)](https://github.com/lirios/screencast)
[![GitHub issues](https://img.shields.io/github/issues/lirios/screencast.svg)](https://github.com/lirios/screencast/issues)
[![CI](https://github.com/lirios/screencast/workflows/CI/badge.svg?branch=develop)](https://github.com/lirios/screencast/actions?query=workflow%3ACI)

Utility to record a video of the screen of a Liri desktop.

## Dependencies

Make sure you have Qt >= 5.10.0 with the following modules:

 * [qtbase](http://code.qt.io/cgit/qt/qtbase.git)

The following modules and their dependencies are required:

 * [cmake](https://gitlab.kitware.com/cmake/cmake) >= 3.10.0
 * [cmake-shared](https://github.com/lirios/cmake-shared.git) >= 1.0.0
 * [gstreamer](https://gitlab.freedesktop.org/gstreamer/gstreamer) >= 1.0.0

## Installation

```sh
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/prefix ..
make
make install # use sudo if necessary
```

Replace `/path/to/prefix` to your installation prefix.
Default is `/usr/local`.

## Licensing

Licensed under the terms of the GNU General Public License version 3.0 or,
at your option, any later version.
