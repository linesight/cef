// Copyright 2026 The Chromium Embedded Framework Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cef/libcef/browser/browser_host_create_util.h"

namespace browser_host_create_util {

bool IsWaylandChild(bool is_wayland_platform,
                    bool is_windowless,
                    bool has_wayland_parent) {
  return is_wayland_platform && !is_windowless && has_wayland_parent;
}

bool IsValidRuntimeStyle(bool is_wayland_child,
                         cef_runtime_style_t runtime_style) {
  return !is_wayland_child || runtime_style == CEF_RUNTIME_STYLE_ALLOY;
}

}  // namespace browser_host_create_util
