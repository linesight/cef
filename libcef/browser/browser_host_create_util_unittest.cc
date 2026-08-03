// Copyright 2026 The Chromium Embedded Framework Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cef/libcef/browser/browser_host_create_util.h"

#include "testing/gtest/include/gtest/gtest.h"

TEST(BrowserHostCreateUtil, WaylandChildRequiresWindowedWaylandWithParent) {
  EXPECT_TRUE(browser_host_create_util::IsWaylandChild(
      /*is_wayland_platform=*/true, /*is_windowless=*/false,
      /*has_wayland_parent=*/true));
  EXPECT_FALSE(browser_host_create_util::IsWaylandChild(
      /*is_wayland_platform=*/false, /*is_windowless=*/false,
      /*has_wayland_parent=*/true));
  EXPECT_FALSE(browser_host_create_util::IsWaylandChild(
      /*is_wayland_platform=*/true, /*is_windowless=*/true,
      /*has_wayland_parent=*/true));
  EXPECT_FALSE(browser_host_create_util::IsWaylandChild(
      /*is_wayland_platform=*/true, /*is_windowless=*/false,
      /*has_wayland_parent=*/false));
}

TEST(BrowserHostCreateUtil, WaylandChildRequiresExplicitAlloyStyle) {
  EXPECT_FALSE(browser_host_create_util::IsValidRuntimeStyle(
      /*is_wayland_child=*/true, CEF_RUNTIME_STYLE_DEFAULT));
  EXPECT_TRUE(browser_host_create_util::IsValidRuntimeStyle(
      /*is_wayland_child=*/true, CEF_RUNTIME_STYLE_ALLOY));
  EXPECT_FALSE(browser_host_create_util::IsValidRuntimeStyle(
      /*is_wayland_child=*/true, CEF_RUNTIME_STYLE_CHROME));
}

TEST(BrowserHostCreateUtil, NonWaylandChildAllowsAllRuntimeStyles) {
  EXPECT_TRUE(browser_host_create_util::IsValidRuntimeStyle(
      /*is_wayland_child=*/false, CEF_RUNTIME_STYLE_DEFAULT));
  EXPECT_TRUE(browser_host_create_util::IsValidRuntimeStyle(
      /*is_wayland_child=*/false, CEF_RUNTIME_STYLE_ALLOY));
  EXPECT_TRUE(browser_host_create_util::IsValidRuntimeStyle(
      /*is_wayland_child=*/false, CEF_RUNTIME_STYLE_CHROME));
}
