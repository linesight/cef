// Copyright 2014 The Chromium Embedded Framework Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef CEF_LIBCEF_BROWSER_NATIVE_NATIVE_WIDGET_DELEGATE_H_
#define CEF_LIBCEF_BROWSER_NATIVE_NATIVE_WIDGET_DELEGATE_H_
#pragma once

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/views/widget/widget_delegate.h"

namespace content {
class WebContents;
}

namespace views {
class FrameView;
class WebView;
class Widget;
}  // namespace views

class CefNativeContentsView;

// Manages the views-based root window that hosts the web contents. This object
// will be deleted when the associated root window is destroyed.
class CefNativeWidgetDelegate : public views::WidgetDelegate {
 public:
  CefNativeWidgetDelegate(SkColor background_color,
                          bool always_on_top,
                          base::RepeatingClosure on_bounds_changed,
                          base::OnceClosure on_delete);

  CefNativeWidgetDelegate(const CefNativeWidgetDelegate&) = delete;
  CefNativeWidgetDelegate& operator=(const CefNativeWidgetDelegate&) = delete;

  // Create the Widget and associated root window. Exactly one parent is used:
  // |parent_widget| for the X11 backend (the browser is hosted as a child of
  // that native window), or |parent_wayland_surface| (a wl_surface*) for the
  // Ozone/Wayland backend (the browser is embedded as a top-level widget whose
  // WaylandWindow roots itself as a wl_subsurface of that external surface).
  // See CEF issue #2804.
  void Init(gfx::AcceleratedWidget parent_widget,
            void* parent_wayland_surface,
            content::WebContents* web_contents,
            const gfx::Rect& bounds);

 private:
  friend class CefNativeContentsView;

  // WidgetDelegate methods:
  bool CanMaximize() const override { return true; }
  bool MaybeGetMinimumSize(gfx::Size* size) const override;
  views::View* GetContentsView() override { return contents_view_.get(); }
  void WidgetIsZombie(views::Widget* widget) override;
  // On the Ozone/Wayland embedding path the widget is a TYPE_WINDOW toplevel,
  // for which Views would otherwise draw a client-side window frame (title bar
  // + caption buttons) inside our wl_subsurface -- making the browser look like
  // a nested window and insetting the web contents. The subsurface has no OS
  // decorations, so use a frameless NativeFrameView: the contents fill the
  // whole surface. See CEF issue #2804.
  std::unique_ptr<views::FrameView> CreateFrameView(
      views::Widget* widget) override;

  const SkColor background_color_;
  const bool always_on_top_;
  // True on the Ozone/Wayland embedding path; suppresses the Views window
  // frame.
  bool frameless_ = false;
  base::RepeatingClosure on_bounds_changed_;
  base::OnceClosure on_delete_;

  raw_ptr<views::View> contents_view_ = nullptr;
  raw_ptr<views::WebView> web_view_ = nullptr;

  std::unique_ptr<views::Widget> widget_;
};

#endif  // CEF_LIBCEF_BROWSER_NATIVE_NATIVE_WIDGET_DELEGATE_H_
