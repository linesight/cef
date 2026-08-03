// Copyright 2026 The Chromium Embedded Framework Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CEF_LIBCEF_BROWSER_BROWSER_HOST_CREATE_UTIL_H_
#define CEF_LIBCEF_BROWSER_BROWSER_HOST_CREATE_UTIL_H_
#pragma once

#include "cef/include/internal/cef_types_runtime.h"

namespace browser_host_create_util {

// Returns true if the inputs request creation of a windowed Wayland child.
bool IsWaylandChild(bool is_wayland_platform,
                    bool is_windowless,
                    bool has_wayland_parent);

// Returns true if |runtime_style| is supported for the specified browser.
// Wayland child browsers currently require explicit Alloy runtime style.
bool IsValidRuntimeStyle(bool is_wayland_child,
                         cef_runtime_style_t runtime_style);

}  // namespace browser_host_create_util

#endif  // CEF_LIBCEF_BROWSER_BROWSER_HOST_CREATE_UTIL_H_
