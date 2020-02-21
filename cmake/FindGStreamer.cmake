#
# SPDX-FileCopyrightText: 2020 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

find_package(PkgConfig)
pkg_check_modules(GStreamer gstreamer-1.0 REQUIRED IMPORTED_TARGET)
