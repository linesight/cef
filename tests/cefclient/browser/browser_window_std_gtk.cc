// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "tests/cefclient/browser/browser_window_std_gtk.h"

#include <X11/Xlib.h>
#include <gdk/gdk.h>
#include <gdk/gdkwayland.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <cstdlib>
#undef Success     // Definition conflicts with cef_message_router.h
#undef RootWindow  // Definition conflicts with root_window.h

#include "include/base/cef_logging.h"
#include "tests/cefclient/browser/client_handler_std.h"
#include "tests/cefclient/browser/util_gtk.h"
#include "tests/shared/browser/main_message_loop.h"

namespace client {

namespace {

::Window GetXWindowForWidget(GtkWidget* widget) {
  ScopedGdkThreadsEnter scoped_gdk_threads;

  // The GTK window must be visible before we can retrieve the XID.
  ::Window xwindow = GDK_WINDOW_XID(gtk_widget_get_window(widget));
  DCHECK(xwindow);
  return xwindow;
}

void SetXWindowVisible(XDisplay* xdisplay, ::Window xwindow, bool visible) {
  CHECK(xdisplay != 0);

  // Retrieve the atoms required by the below XChangeProperty call.
  const char* kAtoms[] = {"_NET_WM_STATE", "ATOM", "_NET_WM_STATE_HIDDEN"};
  Atom atoms[3];
  int result =
      XInternAtoms(xdisplay, const_cast<char**>(kAtoms), 3, false, atoms);
  if (!result) {
    NOTREACHED();
  }

  if (!visible) {
    // Set the hidden property state value.
    auto data = std::make_unique<Atom[]>(1);
    data[0] = atoms[2];

    XChangeProperty(xdisplay, xwindow,
                    atoms[0],  // name
                    atoms[1],  // type
                    32,        // size in bits of items in 'value'
                    PropModeReplace,
                    reinterpret_cast<const unsigned char*>(data.get()),
                    1);  // num items
  } else {
    // Set an empty array of property state values.
    XChangeProperty(xdisplay, xwindow,
                    atoms[0],  // name
                    atoms[1],  // type
                    32,        // size in bits of items in 'value'
                    PropModeReplace, nullptr,
                    0);  // num items
  }
}

void SetXWindowBounds(XDisplay* xdisplay,
                      ::Window xwindow,
                      int x,
                      int y,
                      size_t width,
                      size_t height) {
  CHECK(xdisplay != 0);
  XWindowChanges changes = {0};
  changes.x = x;
  changes.y = y;
  changes.width = static_cast<int>(width);
  changes.height = static_cast<int>(height);
  XConfigureWindow(xdisplay, xwindow, CWX | CWY | CWHeight | CWWidth, &changes);
}

}  // namespace

BrowserWindowStdGtk::BrowserWindowStdGtk(Delegate* delegate,
                                         bool with_controls,
                                         const std::string& startup_url)
    : BrowserWindow(delegate), xdisplay_(nullptr) {
  client_handler_ = new ClientHandlerStd(this, with_controls, startup_url);
}

void BrowserWindowStdGtk::set_xdisplay(XDisplay* xdisplay) {
  REQUIRE_MAIN_THREAD();
  DCHECK(!xdisplay_);
  xdisplay_ = xdisplay;
}

void BrowserWindowStdGtk::OnBrowserCreated(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();

  // Wayland embedding (#2804): the browser renders into a wl_subsurface of
  // |wayland_parent_|'s wl_surface. The subsurface's initial addition and
  // position are double-buffered against that parent surface and only take
  // effect when the parent surface is committed. libcef intentionally does NOT
  // commit the embedder-owned parent surface (that would race/clobber GTK's own
  // surface state), so trigger the commit here by invalidating the parent GDK
  // window: GTK then repaints and commits it, latching the subsurface. This
  // mirrors Firefox's MozContainerWayland approach.
  if (wayland_parent_) {
    ScopedGdkThreadsEnter scoped_gdk_threads;
    if (GdkWindow* window = gtk_widget_get_window(wayland_parent_)) {
      gdk_window_invalidate_rect(window, nullptr, TRUE);
    }
  }

  // Notify the RootWindow only after the parent commit has been requested. A
  // close may have arrived while browser creation was pending, in which case
  // the delegate immediately starts teardown and this object may subsequently
  // be deleted.
  BrowserWindow::OnBrowserCreated(browser);
}

void BrowserWindowStdGtk::CreateBrowser(
    ClientWindowHandle parent_handle,
    const CefRect& rect,
    const CefBrowserSettings& settings,
    CefRefPtr<CefDictionaryValue> extra_info,
    CefRefPtr<CefRequestContext> request_context) {
  REQUIRE_MAIN_THREAD();

  CefWindowInfo window_info;
#if CEF_API_ADDED(CEF_EXPERIMENTAL)
  if (GDK_IS_WAYLAND_DISPLAY(gtk_widget_get_display(parent_handle))) {
    // Wayland embedding: parent the browser to the GTK window's wl_surface as
    // a wl_subsurface. The widget must be realized.
    ScopedGdkThreadsEnter scoped_gdk_threads;
    wl_surface* surface =
        gdk_wayland_window_get_wl_surface(gtk_widget_get_window(parent_handle));
    CHECK(surface);
    window_info.SetAsWaylandChild(surface, rect);
    // Remember the parent widget so OnBrowserCreated() can commit its surface
    // and latch the browser's wl_subsurface. See CEF issue #2804.
    wayland_parent_ = parent_handle;
  } else
#endif
  {
    window_info.SetAsChild(GetXWindowForWidget(parent_handle), rect);
  }

  if (delegate_->UseAlloyStyle()) {
    window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
  }

  CefBrowserHost::CreateBrowser(window_info, client_handler_,
                                client_handler_->startup_url(), settings,
                                extra_info, request_context);
}

void BrowserWindowStdGtk::GetPopupConfig(CefWindowHandle temp_handle,
                                         CefWindowInfo& windowInfo,
                                         CefRefPtr<CefClient>& client,
                                         CefBrowserSettings& settings) {
  CEF_REQUIRE_UI_THREAD();

  // The window will be properly sized after the browser is created.
  windowInfo.SetAsChild(temp_handle, CefRect());

  if (delegate_->UseAlloyStyle()) {
    windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
  }

  client = client_handler_;
}

void BrowserWindowStdGtk::ShowPopup(ClientWindowHandle parent_handle,
                                    int x,
                                    int y,
                                    size_t width,
                                    size_t height) {
  REQUIRE_MAIN_THREAD();

  // X11 reparenting is not applicable to Wayland subsurface embedding.
  if (browser_ && xdisplay_) {
    ::Window parent_xwindow = GetXWindowForWidget(parent_handle);
    CHECK(xdisplay_ != 0);
    ::Window xwindow = browser_->GetHost()->GetWindowHandle();
    DCHECK(xwindow);

    XReparentWindow(xdisplay_, xwindow, parent_xwindow, x, y);

    SetXWindowBounds(xdisplay_, xwindow, x, y, width, height);
    SetXWindowVisible(xdisplay_, xwindow, true);
  }
}

void BrowserWindowStdGtk::Show() {
  REQUIRE_MAIN_THREAD();

  // On Wayland (no |xdisplay_|) the browser is a wl_subsurface already shown by
  // the library; skip the X11 visibility manipulation (CEF issue #2804).
  if (browser_ && xdisplay_) {
    ::Window xwindow = browser_->GetHost()->GetWindowHandle();
    DCHECK(xwindow);
    SetXWindowVisible(xdisplay_, xwindow, true);
  }
}

void BrowserWindowStdGtk::Hide() {
  REQUIRE_MAIN_THREAD();

  if (browser_ && xdisplay_) {
    ::Window xwindow = browser_->GetHost()->GetWindowHandle();
    DCHECK(xwindow);
    SetXWindowVisible(xdisplay_, xwindow, false);
  }
}

void BrowserWindowStdGtk::SetBounds(int x, int y, size_t width, size_t height) {
  REQUIRE_MAIN_THREAD();
  if (!browser_) {
    return;
  }

#if CEF_API_ADDED(CEF_EXPERIMENTAL)
  // Wayland embedding and X11 Alloy browsers use the unified host API. Keep
  // Chrome-style X11 on its existing native path: SetBounds() intentionally
  // rejects Chrome-style browsers, and --wayland-embed must not regress normal
  // native cefclient resizing.
  if (!xdisplay_ || delegate_->UseAlloyStyle()) {
    browser_->GetHost()->SetBounds(
        CefRect(x, y, static_cast<int>(width), static_cast<int>(height)));
    return;
  }
#endif

  if (xdisplay_) {
    ::Window xwindow = browser_->GetHost()->GetWindowHandle();
    DCHECK(xwindow);
    SetXWindowBounds(xdisplay_, xwindow, x, y, width, height);
  }
}

void BrowserWindowStdGtk::SetFocus(bool focus) {
  REQUIRE_MAIN_THREAD();

  if (browser_) {
    browser_->GetHost()->SetFocus(focus);
  }
}

ClientWindowHandle BrowserWindowStdGtk::GetWindowHandle() const {
  REQUIRE_MAIN_THREAD();

  // There is no GtkWidget* representation of this object.
  NOTREACHED();
  return nullptr;
}

}  // namespace client
