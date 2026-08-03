// Copyright 2015 The Chromium Embedded Framework Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cef/libcef/browser/native/browser_platform_delegate_native_linux.h"

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "cef/libcef/browser/alloy/alloy_browser_host_impl.h"
#include "cef/libcef/browser/browser_host_base.h"
#include "cef/libcef/browser/context.h"
#include "cef/libcef/browser/native/native_widget_delegate.h"
#include "cef/libcef/browser/thread_util.h"
#include "components/input/native_web_keyboard_event.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/render_view_host.h"
#include "third_party/blink/public/mojom/renderer_preferences.mojom.h"
#include "ui/events/keycodes/dom/dom_key.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keysym_to_unicode.h"
#include "ui/gfx/font_render_params.h"
#include "ui/views/widget/widget.h"

#if BUILDFLAG(SUPPORTS_OZONE_X11)
#include "cef/libcef/browser/native/window_x11.h"
#include "ui/events/keycodes/keyboard_code_conversion_x.h"
#include "ui/events/keycodes/keyboard_code_conversion_xkb.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_linux.h"
#endif

#if BUILDFLAG(SUPPORTS_OZONE_WAYLAND)
#include "ui/ozone/public/ozone_platform.h"
#endif

CefBrowserPlatformDelegateNativeLinux::CefBrowserPlatformDelegateNativeLinux(
    const CefWindowInfo& window_info,
    SkColor background_color)
    : CefBrowserPlatformDelegateNativeAura(window_info, background_color) {}

void CefBrowserPlatformDelegateNativeLinux::BrowserDestroyed(
    CefBrowserHostBase* browser) {
  CefBrowserPlatformDelegateNativeAura::BrowserDestroyed(browser);

  if (host_window_created_) {
    // Release the reference added in CreateHostWindow().
    browser->Release();
  }
}

bool CefBrowserPlatformDelegateNativeLinux::CreateHostWindow() {
  DCHECK(!window_widget_);

  if (window_info_.bounds.width == 0) {
    window_info_.bounds.width = 800;
  }
  if (window_info_.bounds.height == 0) {
    window_info_.bounds.height = 600;
  }

  gfx::Rect rect(window_info_.bounds.x, window_info_.bounds.y,
                 window_info_.bounds.width, window_info_.bounds.height);

#if BUILDFLAG(SUPPORTS_OZONE_WAYLAND)
  if (!host_window_created_ &&
      ui::OzonePlatform::GetPlatformNameForTest() == "wayland") {
    // Ozone/Wayland embedding (CEF issue #2804). Unlike X11, no intermediate
    // CEF-owned window is created: the browser's own WaylandWindow roots itself
    // as a wl_subsurface of the external parent surface (provided in
    // CefWindowInfo). Wayland has no server-global window-handle equivalent to
    // an X11 XID, so |window| is left null.
    void* parent_wayland_surface = nullptr;
#if CEF_API_ADDED(CEF_EXPERIMENTAL)
    parent_wayland_surface = window_info_.parent_wayland_surface;
#endif
    if (!parent_wayland_surface) {
      LOG(ERROR) << "Native Ozone/Wayland browser creation requires an "
                    "external parent wl_surface";
      return false;
    }
    if (!ui::OzonePlatform::GetInstance()
             ->CanUseExternalWaylandSurfaceForEmbedding(
                 static_cast<wl_surface*>(parent_wayland_surface))) {
      return false;
    }
    window_info_.window = kNullWindowHandle;

    host_window_created_ = true;

    // Add a reference that will be released in BrowserDestroyed().
    browser_->AddRef();

    auto* widget_delegate = new CefNativeWidgetDelegate(
        GetBackgroundColor(), /*always_on_top=*/false,
        GetBoundsChangedCallback(),
        base::BindOnce(
            &CefBrowserPlatformDelegateNativeLinux::WaylandWidgetDeleted,
            weak_ptr_factory_.GetWeakPtr()));
    // Pass the full |rect|: its size sizes the widget while its origin (the
    // browser's position within the host's content area, e.g. below the host's
    // menu/tool bars) becomes the wl_subsurface offset. The widget's own bounds
    // are kept zero-origin inside Init() -- a non-zero widget origin marks it
    // off-root and occlusion tracking stops frame production (CEF #2804).
    widget_delegate->Init(gfx::kNullAcceleratedWidget, parent_wayland_surface,
                          web_contents_, rect);

    window_widget_ = widget_delegate->GetWidget();
    window_widget_->Show();
  }
#endif  // BUILDFLAG(SUPPORTS_OZONE_WAYLAND)

#if BUILDFLAG(SUPPORTS_OZONE_X11)
  if (!host_window_created_) {
    DCHECK(!window_x11_);

    x11::Window parent_window = x11::Window::None;
    if (window_info_.parent_window != kNullWindowHandle) {
      parent_window = static_cast<x11::Window>(window_info_.parent_window);
    }

    // Create a new window object. It will delete itself when the associated X11
    // window is destroyed.
    window_x11_ =
        new CefWindowX11(browser_.get(), parent_window, rect,
                         CefString(&window_info_.window_name).ToString());
    DCHECK_NE(window_x11_->xwindow(), x11::Window::None);
    window_info_.window =
        static_cast<cef_window_handle_t>(window_x11_->xwindow());

    host_window_created_ = true;

    // Add a reference that will be released in BrowserDestroyed().
    browser_->AddRef();

    auto* widget_delegate = new CefNativeWidgetDelegate(
        GetBackgroundColor(), window_x11_->TopLevelAlwaysOnTop(),
        GetBoundsChangedCallback(), GetWidgetDeleteCallback());
    widget_delegate->Init(
        static_cast<gfx::AcceleratedWidget>(window_info_.window),
        /*parent_wayland_surface=*/nullptr, web_contents_,
        gfx::Rect(gfx::Point(), rect.size()));

    window_widget_ = widget_delegate->GetWidget();
    window_widget_->Show();

    window_x11_->Show();
  }
#endif  // BUILDFLAG(SUPPORTS_OZONE_X11)

  // As an additional requirement on Linux, we must set the colors for the
  // render widgets in webkit.
  auto prefs = web_contents_->GetMutableRendererPrefs();
  prefs->focus_ring_color = SkColorSetARGB(255, 229, 151, 0);

  prefs->active_selection_bg_color = SkColorSetRGB(30, 144, 255);
  prefs->active_selection_fg_color = SK_ColorWHITE;
  prefs->inactive_selection_bg_color = SkColorSetRGB(200, 200, 200);
  prefs->inactive_selection_fg_color = SkColorSetRGB(50, 50, 50);

  // Set font-related attributes.
  static const gfx::FontRenderParams params(
      gfx::GetFontRenderParams(gfx::FontRenderParamsQuery(), nullptr));
  prefs->should_antialias_text = params.antialiasing;
  prefs->use_subpixel_positioning = params.subpixel_positioning;
  prefs->hinting = params.hinting;
  prefs->use_autohinter = params.autohinter;
  prefs->use_bitmaps = params.use_bitmaps;
  prefs->subpixel_rendering = params.subpixel_rendering;

  web_contents_->SyncRendererPrefs();

  return true;
}

void CefBrowserPlatformDelegateNativeLinux::CloseHostWindow() {
#if BUILDFLAG(SUPPORTS_OZONE_X11)
  if (window_x11_) {
    window_x11_->Close();
    return;
  }
#endif
#if BUILDFLAG(SUPPORTS_OZONE_WAYLAND)
  // Wayland embedding: no intermediate window; close the hosting widget.
  if (window_widget_) {
    if (wayland_close_pending_) {
      return;
    }
    wayland_close_pending_ = true;

    window_widget_->Close();
  }
#endif
}

void CefBrowserPlatformDelegateNativeLinux::WaylandWidgetDeleted() {
  DCHECK(window_widget_);
  window_widget_ = nullptr;

  if (!browser_ || wayland_window_destroyed_pending_) {
    return;
  }
  wayland_window_destroyed_pending_ = true;

  // AlloyBrowserHostImpl expects destruction of the native host window to
  // complete browser teardown via WindowDestroyed(). The embedded Wayland
  // Widget can disappear either during a requested close or after an Ozone
  // surface-creation failure. Complete both paths here, after the Widget's
  // destruction stack has unwound, and exactly once. See CEF issue #2804.
  CefRefPtr<CefBrowserHostBase> browser(browser_);
  CEF_POST_TASK(
      CEF_UIT,
      base::BindOnce(
          [](CefRefPtr<CefBrowserHostBase> browser) {
            AlloyBrowserHostImpl::FromBaseChecked(browser)->WindowDestroyed();
          },
          browser));
}

CefWindowHandle CefBrowserPlatformDelegateNativeLinux::GetHostWindowHandle()
    const {
  if (windowless_handler_) {
    return windowless_handler_->GetParentWindowHandle();
  }
  return window_info_.window;
}

views::Widget* CefBrowserPlatformDelegateNativeLinux::GetWindowWidget() const {
  return window_widget_;
}

void CefBrowserPlatformDelegateNativeLinux::SetFocus(bool setFocus) {
  if (!setFocus) {
    return;
  }

  if (web_contents_) {
    // Give logical focus to the RenderWidgetHostViewAura in the views
    // hierarchy. This does not change the native keyboard focus.
    web_contents_->Focus();
  }

#if BUILDFLAG(SUPPORTS_OZONE_X11)
  if (window_x11_) {
    // Give native focus to the DesktopNativeWidgetAura for the root window.
    // Needs to be done via the ::Window so that keyboard focus is assigned
    // correctly.
    window_x11_->Focus();
  }
#endif  // BUILDFLAG(SUPPORTS_OZONE_X11)
}

void CefBrowserPlatformDelegateNativeLinux::NotifyMoveOrResizeStarted() {
  // Call the parent method to dismiss any existing popups.
  CefBrowserPlatformDelegateNativeAura::NotifyMoveOrResizeStarted();

  if (!web_contents_) {
    return;
  }

#if BUILDFLAG(SUPPORTS_OZONE_X11)
  if (!window_x11_) {
    return;
  }

  views::DesktopWindowTreeHostLinux* tree_host = window_x11_->GetHost();
  if (!tree_host) {
    return;
  }

  // Explicitly set the screen bounds so that WindowTreeHost::*Screen()
  // methods return the correct results.
  const gfx::Rect& bounds = window_x11_->GetBoundsInScreen();
  tree_host->set_screen_bounds(bounds);

  // Send updated screen rectangle information to the renderer process so that
  // popups are displayed in the correct location.
  content::RenderWidgetHostImpl::From(
      web_contents_->GetRenderViewHost()->GetWidget())
      ->SendScreenRects();
#endif  // BUILDFLAG(SUPPORTS_OZONE_X11)
}

void CefBrowserPlatformDelegateNativeLinux::SetBounds(const gfx::Rect& bounds) {
#if BUILDFLAG(SUPPORTS_OZONE_X11)
  if (window_x11_) {
    if (window_info_.parent_window == kNullWindowHandle) {
      LOG(WARNING) << "CefBrowserHost::SetBounds requires an externally "
                      "parented X11 child window";
      return;
    }
    window_x11_->SetBounds(bounds);
    window_info_.bounds =
        CefRect(bounds.x(), bounds.y(), bounds.width(), bounds.height());
    return;
  }
#endif  // BUILDFLAG(SUPPORTS_OZONE_X11)

#if BUILDFLAG(SUPPORTS_OZONE_WAYLAND)
  if (window_widget_) {
#if CEF_API_ADDED(CEF_EXPERIMENTAL)
    if (!window_info_.parent_wayland_surface) {
      LOG(WARNING) << "CefBrowserHost::SetBounds requires an externally "
                      "parented Wayland child surface";
      return;
    }
#endif

    const gfx::Point initial_origin(window_info_.bounds.x,
                                    window_info_.bounds.y);
    if (bounds.origin() != initial_origin) {
      LOG(WARNING) << "CefBrowserHost::SetBounds does not yet support Wayland "
                      "position changes; preserving initial origin "
                   << initial_origin.ToString();
    }

    // Keep the Aura/widget origin at zero. A non-zero origin marks this
    // top-level Widget as off-root and can stop compositor frame production.
    // Placement is stored independently by WaylandEmbeddedWindow as the
    // wl_subsurface offset.
    if (window_info_.bounds.width != bounds.width() ||
        window_info_.bounds.height != bounds.height()) {
      NotifyMoveOrResizeStarted();
    }
    window_widget_->SetSize(bounds.size());
    window_info_.bounds.width = bounds.width();
    window_info_.bounds.height = bounds.height();
  }
#endif  // BUILDFLAG(SUPPORTS_OZONE_WAYLAND)
}

void CefBrowserPlatformDelegateNativeLinux::SizeTo(int width, int height) {
#if BUILDFLAG(SUPPORTS_OZONE_X11)
  if (window_x11_) {
    window_x11_->SetSize(gfx::Size(width, height));
    return;
  }
#endif  // BUILDFLAG(SUPPORTS_OZONE_X11)
#if BUILDFLAG(SUPPORTS_OZONE_WAYLAND)
  // Wayland embedding: resize the hosting widget while preserving its
  // zero-origin Aura bounds and independent wl_subsurface offset.
  if (window_widget_) {
    window_widget_->SetSize(gfx::Size(width, height));
  }
#endif  // BUILDFLAG(SUPPORTS_OZONE_WAYLAND)
}

void CefBrowserPlatformDelegateNativeLinux::ViewText(const std::string& text) {
  char buff[] = "/tmp/CEFSourceXXXXXX";
  int fd = mkstemp(buff);

  if (fd == -1) {
    return;
  }

  FILE* srcOutput = fdopen(fd, "w+");
  if (!srcOutput) {
    return;
  }

  if (fputs(text.c_str(), srcOutput) < 0) {
    fclose(srcOutput);
    return;
  }

  fclose(srcOutput);

  std::string newName(buff);
  newName.append(".txt");
  if (rename(buff, newName.c_str()) != 0) {
    return;
  }

  std::string openCommand("xdg-open ");
  openCommand += newName;

  [[maybe_unused]] int result = system(openCommand.c_str());
}

bool CefBrowserPlatformDelegateNativeLinux::HandleKeyboardEvent(
    const input::NativeWebKeyboardEvent& event) {
  // TODO(cef): Is something required here to handle shortcut keys?
  return false;
}

CefEventHandle CefBrowserPlatformDelegateNativeLinux::GetEventHandle(
    const input::NativeWebKeyboardEvent& event) const {
  // TODO(cef): We need to return an XEvent* from this method, but
  // |event.os_event->native_event()| now returns a ui::Event* instead.
  // See https://crbug.com/965991.
  return nullptr;
}

ui::KeyEvent CefBrowserPlatformDelegateNativeLinux::TranslateUiKeyEvent(
    const CefKeyEvent& key_event) const {
  int flags = TranslateUiEventModifiers(key_event.modifiers);
  ui::KeyboardCode key_code =
      static_cast<ui::KeyboardCode>(key_event.windows_key_code);
  ui::DomCode dom_code =
      ui::KeycodeConverter::NativeKeycodeToDomCode(key_event.native_key_code);

#if BUILDFLAG(SUPPORTS_OZONE_X11)
  int keysym = ui::XKeysymForWindowsKeyCode(
      key_code, !!(key_event.modifiers & EVENTFLAG_SHIFT_DOWN));
  char16_t character = ui::GetUnicodeCharacterFromXKeySym(keysym);
#else
  char16_t character = key_event.character;
#endif

  base::TimeTicks time_stamp = GetEventTimeStamp();

  if (key_event.type == KEYEVENT_CHAR) {
    return ui::KeyEvent::FromCharacter(character, key_code, dom_code, flags,
                                       time_stamp);
  }

  ui::EventType type = ui::EventType::kUnknown;
  switch (key_event.type) {
    case KEYEVENT_RAWKEYDOWN:
    case KEYEVENT_KEYDOWN:
      type = ui::EventType::kKeyPressed;
      break;
    case KEYEVENT_KEYUP:
      type = ui::EventType::kKeyReleased;
      break;
    default:
      DCHECK(false);
  }

#if BUILDFLAG(SUPPORTS_OZONE_X11)
  ui::DomKey dom_key = ui::XKeySymToDomKey(keysym, character);
#else
  ui::DomKey dom_key = ui::DomKey::NONE;
#endif

  return ui::KeyEvent(type, key_code, dom_code, flags, dom_key, time_stamp);
}

input::NativeWebKeyboardEvent
CefBrowserPlatformDelegateNativeLinux::TranslateWebKeyEvent(
    const CefKeyEvent& key_event) const {
  ui::KeyEvent ui_event = TranslateUiKeyEvent(key_event);
  if (key_event.type == KEYEVENT_CHAR) {
    return input::NativeWebKeyboardEvent(ui_event, key_event.character);
  }
  return input::NativeWebKeyboardEvent(ui_event);
}
