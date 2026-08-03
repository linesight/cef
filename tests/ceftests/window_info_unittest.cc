// Copyright 2026 The Chromium Embedded Framework Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "include/cef_browser.h"
#include "tests/gtest/include/gtest/gtest.h"

#if CEF_API_ADDED(CEF_EXPERIMENTAL)

namespace {

CefWaylandSurfaceHandle GetFakeWaylandParent() {
  static int value;
  return reinterpret_cast<CefWaylandSurfaceHandle>(&value);
}

}  // namespace

TEST(WindowInfoTest, SetAsWaylandChildSelectsAlloyStyle) {
  CefWindowInfo window_info;
  EXPECT_EQ(CEF_RUNTIME_STYLE_DEFAULT, window_info.runtime_style);

  const CefRect bounds(1, 2, 300, 200);
  const auto parent = GetFakeWaylandParent();
  window_info.SetAsWaylandChild(parent, bounds);

  EXPECT_EQ(parent, window_info.parent_wayland_surface);
  EXPECT_EQ(bounds, window_info.bounds);
  EXPECT_EQ(CEF_RUNTIME_STYLE_ALLOY, window_info.runtime_style);
}

#endif  // CEF_API_ADDED(CEF_EXPERIMENTAL)
