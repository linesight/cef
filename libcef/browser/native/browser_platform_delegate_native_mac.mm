// Copyright 2015 The Chromium Embedded Framework Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cef/libcef/browser/native/browser_platform_delegate_native_mac.h"

#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

#include "base/apple/owned_objc.h"
#include "base/apple/scoped_nsautorelease_pool.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/notimplemented.h"
#include "base/threading/thread_restrictions.h"
#include "cef/include/internal/cef_types_mac.h"
#include "cef/libcef/browser/alloy/alloy_browser_host_impl.h"
#include "cef/libcef/browser/browser_event_util.h"
#include "cef/libcef/browser/context.h"
#include "cef/libcef/browser/key_event_util.h"
#include "cef/libcef/browser/native/javascript_dialog_runner_mac.h"
#include "cef/libcef/browser/native/menu_runner_mac.h"
#include "cef/libcef/browser/thread_util.h"
#include "components/input/native_web_keyboard_event.h"
#include "content/browser/renderer_host/render_widget_host_view_mac.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "third_party/blink/public/common/input/web_mouse_wheel_event.h"
#include "ui/display/screen.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/keycodes/keyboard_codes_posix.h"
#include "ui/gfx/geometry/rect.h"

// Wrapper NSView for the native view. Necessary to destroy the browser when
// the view is deleted or, for browsers parented to a client-provided view,
// when the containing window closes.
@interface CefBrowserHostView : NSView {
 @private
  CefBrowserHostBase* browser_;  // weak
  BOOL destroyOnWindowClose_;
  NSWindow* __weak observedWindow_;
  // Set when CEF asks to close the host window (after DoClose() returns
  // false). |hostWindow_| is the window that contained this view at that time.
  BOOL closeRequested_;
  NSWindow* __weak hostWindow_;
  // Set when browser destruction has been scheduled.
  BOOL destroyScheduled_;
  // Keeps a closing window, and therefore the client-provided parent view,
  // alive until the scheduled destruction runs.
  NSWindow* __strong closingWindow_;
}

@property(nonatomic, assign) CefBrowserHostBase* browser;

// When YES the browser is destroyed when the window containing this view
// closes instead of waiting for this view to deallocate, which may never
// happen while the client references the window. Used for browsers parented
// to a client-provided view, which have no CefWindowDelegate. This matches
// Chromium, which treats NSWindowWillCloseNotification as window destruction
// (see ui/native_window_tracker/native_window_tracker_cocoa.mm).
@property(nonatomic, assign) BOOL destroyOnWindowClose;

// Called when the browser is destroyed by some other means (for example,
// during CefShutdown) so that this view no longer references it.
- (void)browserDestroyed;

// Called when CEF needs to close the host window after DoClose() returned
// false. Only used when |destroyOnWindowClose| is YES. Closes the window that
// contains this view, or destroys the browser if there is none. If this view
// later leaves that window the browser is destroyed instead, so CEF never
// closes a window that it was not asked to close.
- (void)closeHostWindow;

@end

@implementation CefBrowserHostView

@synthesize browser = browser_;
@synthesize destroyOnWindowClose = destroyOnWindowClose_;

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  if (browser_) {
    // Force the browser to be destroyed and release the reference added in
    // PlatformCreateWindow().
    AlloyBrowserHostImpl::FromBaseChecked(browser_)->WindowDestroyed();
  }
}

- (void)browserDestroyed {
  browser_ = nullptr;
  destroyOnWindowClose_ = NO;
  closingWindow_ = nil;
  [self observeWindow:nil];
}

- (void)setDestroyOnWindowClose:(BOOL)destroyOnWindowClose {
  destroyOnWindowClose_ = destroyOnWindowClose;
  [self observeWindow:destroyOnWindowClose_ ? [self window] : nil];
}

- (void)viewWillMoveToWindow:(NSWindow*)newWindow {
  [super viewWillMoveToWindow:newWindow];
  if (destroyOnWindowClose_) {
    [self observeWindow:newWindow];
  }
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  if (destroyOnWindowClose_ && closeRequested_ &&
      [self window] != hostWindow_) {
    // The view left the host window after CEF asked to close it (for example,
    // the client removed it or moved it to another window).
    [self scheduleDestroy];
  }
}

- (void)closeHostWindow {
  if (!browser_ || closeRequested_) {
    return;
  }
  closeRequested_ = YES;
  hostWindow_ = [self window];
  // Continue after clearing the stack.
  [self performSelectorOnMainThread:@selector(continueClose)
                         withObject:nil
                      waitUntilDone:NO];
}

- (void)continueClose {
  if (!browser_ || destroyScheduled_) {
    return;
  }
  if (hostWindow_ && [self window] == hostWindow_) {
    // Destruction completes when the window closes (see windowWillClose:).
    [hostWindow_ performClose:nil];
    return;
  }
  // There is no host window to close, so destroy the browser directly
  // instead of waiting for a window close or view deallocation that may never
  // happen.
  [self destroyBrowser];
}

- (void)scheduleDestroy {
  if (!browser_ || destroyScheduled_) {
    return;
  }
  destroyScheduled_ = YES;
  // Destroy the browser after clearing the stack of anything that might have
  // the window on it. Matches the behavior of CefWindowDelegate.
  [self performSelectorOnMainThread:@selector(destroyBrowser)
                         withObject:nil
                      waitUntilDone:NO];
}

- (void)observeWindow:(NSWindow*)window {
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  NSWindow* oldWindow = observedWindow_;
  if (oldWindow == window) {
    return;
  }
  if (oldWindow) {
    [center removeObserver:self
                      name:NSWindowWillCloseNotification
                    object:oldWindow];
  }
  observedWindow_ = window;
  if (window) {
    [center addObserver:self
               selector:@selector(windowWillClose:)
                   name:NSWindowWillCloseNotification
                 object:window];
  }
}

- (void)windowWillClose:(NSNotification*)notification {
  // The window that contains this view is closing.
  if (browser_ && !closingWindow_) {
    closingWindow_ = notification.object;
  }
  [self scheduleDestroy];
}

- (void)destroyBrowser {
  // The browser may already have been destroyed by other means (for example,
  // during CefShutdown) before scheduled destruction runs.
  if (!browser_) {
    closingWindow_ = nil;
    return;
  }
  [self observeWindow:nil];
  CefRefPtr<AlloyBrowserHostImpl> browser =
      AlloyBrowserHostImpl::FromBaseChecked(browser_);
  // Prevent dealloc from notifying the destroyed browser again later.
  browser_ = nullptr;
  browser->WindowDestroyed();
  closingWindow_ = nil;
}

@end

// Receives notifications from the browser window. Will delete itself when done.
@interface CefWindowDelegate : NSObject <NSWindowDelegate> {
 @private
  CefBrowserHostBase* browser_;  // weak
  NSWindow* __strong window_;
}
- (id)initWithWindow:(NSWindow*)window andBrowser:(CefBrowserHostBase*)browser;
- (void)browserDestroyed;
@end

@implementation CefWindowDelegate

- (id)initWithWindow:(NSWindow*)window andBrowser:(CefBrowserHostBase*)browser {
  if (self = [super init]) {
    window_ = window;
    browser_ = browser;
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)browserDestroyed {
  browser_ = nullptr;
}

- (BOOL)windowShouldClose:(id)window {
  if (browser_ && !browser_->TryCloseBrowser()) {
    // Cancel the close.
    return NO;
  }

  // Allow the close.
  return YES;
}

- (void)windowWillClose:(NSNotification*)notification {
  // For an NSWindow object, the default is to be released on |close|. We
  // instead want it to remain valid until all strong references are released
  // via |cleanup:| and |BrowserDestroyed|.
  window_.releasedWhenClosed = NO;

  // Clean ourselves up after clearing the stack of anything that might have the
  // window on it.
  [self performSelectorOnMainThread:@selector(cleanup:)
                         withObject:notification.object
                      waitUntilDone:NO];
}

- (void)cleanup:(id)window {
  [window_ setDelegate:nil];
  if (browser_) {
    CefRefPtr<AlloyBrowserHostImpl> browser =
        AlloyBrowserHostImpl::FromBaseChecked(browser_);
    CefBrowserHostView* browser_view =
        (CefBrowserHostView*)CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(
            browser->GetWindowHandle());
    // AppKit or the embedder may retain the view after the window closes.
    // Destroy the browser now instead of waiting for the view to deallocate,
    // and prevent the view from notifying the destroyed browser again later.
    browser_view.browser = nullptr;
    browser_ = nullptr;
    browser->WindowDestroyed();
  }
  window_ = nil;
}

@end

namespace {

NSTimeInterval currentEventTimestamp() {
  NSEvent* currentEvent = [NSApp currentEvent];
  if (currentEvent) {
    return [currentEvent timestamp];
  } else {
    // FIXME(API): In case there is no current event, the timestamp could be
    // obtained by getting the time since the application started. This involves
    // taking some more static functions from Chromium code.
    // Another option is to have the timestamp as a field in CefEvent structures
    // and let the client provide it.
    return 0;
  }
}

constexpr int kDefaultHeight = 750;
constexpr int kDefaultWidth = 750;
constexpr NSWindowStyleMask kDefaultStyleMask =
    NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
    NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

// Keep the frame bounds inside the display work area.
NSRect ClampNSBoundsToWorkArea(const NSRect& frame_bounds,
                               const gfx::Rect& display_bounds,
                               const gfx::Rect& work_area) {
  NSRect bounds = frame_bounds;

  // Convert from DIP coordinates (top-left origin) to macOS coordinates
  // (bottom-left origin).
  const int work_area_y =
      display_bounds.height() - work_area.height() - work_area.y();

  if (bounds.size.width > work_area.width()) {
    bounds.size.width = work_area.width();
  }
  if (bounds.size.height > work_area.height()) {
    bounds.size.height = work_area.height();
  }

  if (bounds.origin.x < work_area.x()) {
    bounds.origin.x = work_area.x();
  } else if (bounds.origin.x + bounds.size.width >=
             work_area.x() + work_area.width()) {
    bounds.origin.x = work_area.x() + work_area.width() - bounds.size.width;
  }

  if (bounds.origin.y < work_area_y) {
    bounds.origin.y = work_area_y;
  } else if (bounds.origin.y + bounds.size.height >=
             work_area_y + work_area.height()) {
    bounds.origin.y = work_area_y + work_area.height() - bounds.size.height;
  }

  return bounds;
}

// Get frame and content area rects matching the input DIP screen bounds. The
// resulting window frame will be kept inside the closest display work area. If
// |input_content_bounds| is true the input size is used for the content area
// and the input origin is used for the frame. Otherwise, both input size and
// origin are used for the frame.
void GetNSBoundsInDisplay(const gfx::Rect& dip_bounds,
                          bool input_content_bounds,
                          NSWindowStyleMask style_mask,
                          NSRect& frame_rect,
                          NSRect& content_rect) {
  // Identify the closest display.
  const auto display = display::Screen::Get()->GetDisplayMatching(dip_bounds);
  const auto& display_bounds = display.bounds();
  const auto& display_work_area = display.work_area();

  // Convert from DIP coordinates (top-left origin) to macOS coordinates
  // (bottom-left origin).
  NSRect requested_rect = NSMakeRect(dip_bounds.x(), dip_bounds.y(),
                                     dip_bounds.width(), dip_bounds.height());
  requested_rect.origin.y = display_bounds.height() -
                            requested_rect.size.height -
                            requested_rect.origin.y;

  // Calculate the equivalent frame and content bounds.
  if (input_content_bounds) {
    // Compute frame rect from content rect. Keep the requested origin.
    content_rect = requested_rect;
    frame_rect = [NSWindow frameRectForContentRect:content_rect
                                         styleMask:style_mask];
    frame_rect.origin = requested_rect.origin;
  } else {
    // Compute content rect from frame rect.
    frame_rect = requested_rect;
    content_rect = [NSWindow contentRectForFrameRect:frame_rect
                                           styleMask:style_mask];
  }

  // Keep the frame inside the display work area.
  const NSRect new_frame_rect =
      ClampNSBoundsToWorkArea(frame_rect, display_bounds, display_work_area);
  if (!NSEqualRects(frame_rect, new_frame_rect)) {
    frame_rect = new_frame_rect;
    content_rect = [NSWindow contentRectForFrameRect:frame_rect
                                           styleMask:style_mask];
  }
}

}  // namespace

struct CefBrowserPlatformDelegateNativeMac::ObjCStorage {
  // Weak so that it reads as nil while the view is deallocating.
  CefBrowserHostView* __weak host_view = nil;
};

CefBrowserPlatformDelegateNativeMac::CefBrowserPlatformDelegateNativeMac(
    const CefWindowInfo& window_info,
    SkColor background_color)
    : CefBrowserPlatformDelegateNative(window_info, background_color),
      objc_storage_(std::make_unique<ObjCStorage>()) {}

CefBrowserPlatformDelegateNativeMac::~CefBrowserPlatformDelegateNativeMac() =
    default;

void CefBrowserPlatformDelegateNativeMac::BrowserDestroyed(
    CefBrowserHostBase* browser) {
  CefBrowserPlatformDelegateNative::BrowserDestroyed(browser);

  if (host_window_created_) {
    // Clear the view's reference before releasing the browser. |host_view| is
    // nil if this is called while the view is deallocating.
    [objc_storage_->host_view browserDestroyed];
    objc_storage_->host_view = nil;

    // Release the references added in CreateHostWindow().
    browser->Release();
    [window_delegate_ browserDestroyed];
    window_delegate_ = nil;
  }
}

bool CefBrowserPlatformDelegateNativeMac::CreateHostWindow() {
  base::apple::ScopedNSAutoreleasePool autorelease_pool;

  NSWindow* new_window = nil;

  NSView* parent_view =
      CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(window_info_.parent_view);
  NSRect browser_view_rect =
      NSMakeRect(window_info_.bounds.x, window_info_.bounds.y,
                 window_info_.bounds.width, window_info_.bounds.height);

  if (parent_view == nil) {
    // TODO(port): If no x,y position is specified the window will always appear
    // in the upper-left corner. Maybe there's a better default place to put it?
    const gfx::Rect dip_bounds(
        window_info_.bounds.x, window_info_.bounds.y,
        window_info_.bounds.width <= 0 ? kDefaultWidth
                                       : window_info_.bounds.width,
        window_info_.bounds.height <= 0 ? kDefaultHeight
                                        : window_info_.bounds.height);

    // Calculate the equivalent frame and content area bounds.
    NSRect frame_rect, content_rect;
    GetNSBoundsInDisplay(dip_bounds, /*input_content_bounds=*/true,
                         kDefaultStyleMask, frame_rect, content_rect);

    // Create a new window.
    new_window = [[NSWindow alloc] initWithContentRect:content_rect
                                             styleMask:kDefaultStyleMask
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];

    // Create the delegate for control and browser window events.
    // Add a reference that will be released in BrowserDestroyed().
    window_delegate_ = [[CefWindowDelegate alloc] initWithWindow:new_window
                                                      andBrowser:browser_];
    [new_window setDelegate:window_delegate_];

    parent_view = [new_window contentView];
    browser_view_rect = [parent_view bounds];

    window_info_.parent_view = CAST_NSVIEW_TO_CEF_WINDOW_HANDLE(parent_view);

    // Make the content view for the window have a layer. This will make all
    // sub-views have layers. This is necessary to ensure correct layer
    // ordering of all child views and their layers.
    [parent_view setWantsLayer:YES];

    // Place the window at the target point. This is required for proper
    // placement if the point is on a secondary display.
    [new_window setFrameOrigin:frame_rect.origin];
  }

  host_window_created_ = true;

  // Add a reference that will be released in BrowserDestroyed().
  browser_->AddRef();

  // Create the browser view.
  CefBrowserHostView* browser_view =
      [[CefBrowserHostView alloc] initWithFrame:browser_view_rect];
  browser_view.browser = browser_;
  if (!new_window) {
    // The client owns the window. Destroy the browser when that window closes
    // because there is no CefWindowDelegate to do so (see issue #3810).
    browser_view.destroyOnWindowClose = YES;
  }
  [parent_view addSubview:browser_view];
  [browser_view setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
  [browser_view setNeedsDisplay:YES];

  // Parent the WebContents to the browser view.
  const NSRect bounds = [browser_view bounds];
  NSView* native_view = web_contents_->GetNativeView().GetNativeNSView();
  [browser_view addSubview:native_view];
  [native_view setFrame:bounds];
  [native_view setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
  [native_view setNeedsDisplay:YES];

  window_info_.view = CAST_NSVIEW_TO_CEF_WINDOW_HANDLE(browser_view);
  objc_storage_->host_view = browser_view;

  if (new_window != nil && !window_info_.hidden) {
    // Show the window.
    [new_window makeKeyAndOrderFront:nil];
  }

  return true;
}

void CefBrowserPlatformDelegateNativeMac::CloseHostWindow() {
  NSView* nsview = CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(window_info_.view);
  if (nsview == nil) {
    return;
  }
  CefBrowserHostView* host_view = objc_storage_->host_view;
  if (host_view.destroyOnWindowClose) {
    // The client owns the window. The view may have been removed from it, or
    // moved to another window, so let the view decide how to complete the
    // close.
    [host_view closeHostWindow];
    return;
  }
  [[nsview window] performSelectorOnMainThread:@selector(performClose:)
                                    withObject:nil
                                 waitUntilDone:NO];
}

CefWindowHandle CefBrowserPlatformDelegateNativeMac::GetHostWindowHandle()
    const {
  if (windowless_handler_) {
    return windowless_handler_->GetParentWindowHandle();
  }
  return window_info_.view;
}

void CefBrowserPlatformDelegateNativeMac::SendKeyEvent(
    const CefKeyEvent& event) {
  auto view = GetHostView();
  if (!view) {
    return;
  }

  input::NativeWebKeyboardEvent web_event = TranslateWebKeyEvent(event);
  view->ForwardKeyboardEvent(web_event, ui::LatencyInfo());
}

void CefBrowserPlatformDelegateNativeMac::SendMouseClickEvent(
    const CefMouseEvent& event,
    CefBrowserHost::MouseButtonType type,
    bool mouseUp,
    int clickCount) {
  auto view = GetHostView();
  if (!view) {
    return;
  }

  blink::WebMouseEvent web_event =
      TranslateWebClickEvent(event, type, mouseUp, clickCount);
  view->RouteOrProcessMouseEvent(web_event);
}

void CefBrowserPlatformDelegateNativeMac::SendMouseMoveEvent(
    const CefMouseEvent& event,
    bool mouseLeave) {
  auto view = GetHostView();
  if (!view) {
    return;
  }

  blink::WebMouseEvent web_event = TranslateWebMoveEvent(event, mouseLeave);
  view->RouteOrProcessMouseEvent(web_event);
}

void CefBrowserPlatformDelegateNativeMac::SendWheelEvent(
    const CefWheelInputEvent& event) {
  auto* view = GetHostView();
  if (!view || view->destroy_pending()) {
    return;
  }
  blink::WebMouseWheelEvent web_event;
  if (!GetWebWheelInputEvent(event, web_event)) {
    LOG(WARNING) << "SendWheelEvent: Invalid input event";
    return;
  }
  // Preserve Chromium's phase synthesis and momentum handoff without
  // reconstructing a native event or losing the portable snapshot fields.
  view->RouteOrProcessWheelEvent(web_event);
}

void CefBrowserPlatformDelegateNativeMac::SendMouseWheelEvent(
    const CefMouseEvent& event,
    int deltaX,
    int deltaY) {
  auto view = GetHostView();
  if (!view) {
    return;
  }

  blink::WebMouseWheelEvent web_event =
      TranslateWebWheelEvent(event, deltaX, deltaY);
  view->RouteOrProcessWheelEvent(web_event);
}

void CefBrowserPlatformDelegateNativeMac::SendWebTouchEvent(
    const blink::WebTouchEvent& event) {
  if (auto* view = GetHostView(); view && !view->destroy_pending()) {
    view->RouteOrProcessTouchEvent(event);
  }
}

void CefBrowserPlatformDelegateNativeMac::SetFocus(bool setFocus) {
  auto view = GetHostView();
  if (view) {
    view->SetActive(setFocus);

    if (setFocus) {
      // Give keyboard focus to the native view.
      NSView* nsview = web_contents_->GetContentNativeView().GetNativeNSView();
      DCHECK([nsview canBecomeKeyView]);
      [[nsview window] makeFirstResponder:nsview];
    }
  }
}

gfx::Point CefBrowserPlatformDelegateNativeMac::GetScreenPoint(
    const gfx::Point& view,
    bool want_dip_coords) const {
  // Mac always operates in DIP coordinates so |want_dip_coords| is ignored.
  if (windowless_handler_) {
    return windowless_handler_->GetParentScreenPoint(view, want_dip_coords);
  }

  NSView* nsview = CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(window_info_.parent_view);
  if (nsview) {
    NSRect bounds = [nsview bounds];
    NSPoint view_pt = {static_cast<CGFloat>(view.x()),
                       bounds.size.height - static_cast<CGFloat>(view.y())};
    NSPoint window_pt = [nsview convertPoint:view_pt toView:nil];
    NSPoint screen_pt = [[nsview window] convertPointToScreen:window_pt];
    return gfx::Point(screen_pt.x, screen_pt.y);
  }
  return gfx::Point();
}

void CefBrowserPlatformDelegateNativeMac::ViewText(const std::string& text) {
  // TODO(cef): Implement this functionality.
  NOTIMPLEMENTED();
}

bool CefBrowserPlatformDelegateNativeMac::HandleKeyboardEvent(
    const input::NativeWebKeyboardEvent& event) {
  // Give the top level menu equivalents a chance to handle the event.
  NSEvent* ns_event = event.os_event.Get();
  if (ns_event.type == NSEventTypeKeyDown) {
    return [[NSApp mainMenu] performKeyEquivalent:ns_event];
  }
  return false;
}

CefEventHandle CefBrowserPlatformDelegateNativeMac::GetEventHandle(
    const input::NativeWebKeyboardEvent& event) const {
  return CAST_NSEVENT_TO_CEF_EVENT_HANDLE(event.os_event.Get());
}

std::unique_ptr<CefJavaScriptDialogRunner>
CefBrowserPlatformDelegateNativeMac::CreateJavaScriptDialogRunner() {
  return base::WrapUnique(new CefJavaScriptDialogRunnerMac);
}

std::unique_ptr<CefMenuRunner>
CefBrowserPlatformDelegateNativeMac::CreateMenuRunner() {
  return base::WrapUnique(new CefMenuRunnerMac);
}

bool CefBrowserPlatformDelegateNativeMac::IsPrintPreviewSupported() const {
  // MacOS with external parent can't support print preview because there is no
  // gfx::NativeView or gfx::AcceleratedWidget. See related comments in
  // AlloyWebContentsDialogHelper.
  return false;
}

input::NativeWebKeyboardEvent
CefBrowserPlatformDelegateNativeMac::TranslateWebKeyboardInputEvent(
    const blink::WebKeyboardEvent& event) const {
  return CreateNativeKeyboardInputEvent(
      event,
      gfx::NativeView(CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(GetHostWindowHandle())));
}

// DEPRECATED: Legacy SendKeyEvent conversion; use
// TranslateWebKeyboardInputEvent for unified keyboard input.
input::NativeWebKeyboardEvent
CefBrowserPlatformDelegateNativeMac::TranslateWebKeyEvent(
    const CefKeyEvent& key_event) const {
  return GetLegacyWebKeyEvent(key_event, currentEventTimestamp());
}

blink::WebMouseEvent
CefBrowserPlatformDelegateNativeMac::TranslateWebClickEvent(
    const CefMouseEvent& mouse_event,
    CefBrowserHost::MouseButtonType type,
    bool mouseUp,
    int clickCount) const {
  blink::WebMouseEvent result;
  TranslateWebMouseEvent(result, mouse_event);

  switch (type) {
    case CEF_MOUSE_BUTTON_LEFT:
      result.SetType(mouseUp ? blink::WebInputEvent::Type::kMouseUp
                             : blink::WebInputEvent::Type::kMouseDown);
      result.button = blink::WebMouseEvent::Button::kLeft;
      break;
    case CEF_MOUSE_BUTTON_MIDDLE:
      result.SetType(mouseUp ? blink::WebInputEvent::Type::kMouseUp
                             : blink::WebInputEvent::Type::kMouseDown);
      result.button = blink::WebMouseEvent::Button::kMiddle;
      break;
    case CEF_MOUSE_BUTTON_RIGHT:
      result.SetType(mouseUp ? blink::WebInputEvent::Type::kMouseUp
                             : blink::WebInputEvent::Type::kMouseDown);
      result.button = blink::WebMouseEvent::Button::kRight;
      break;
    case CEF_MOUSE_BUTTON_BACK:
      result.SetType(mouseUp ? blink::WebInputEvent::Type::kMouseUp
                             : blink::WebInputEvent::Type::kMouseDown);
      result.button = blink::WebMouseEvent::Button::kBack;
      break;
    case CEF_MOUSE_BUTTON_FORWARD:
      result.SetType(mouseUp ? blink::WebInputEvent::Type::kMouseUp
                             : blink::WebInputEvent::Type::kMouseDown);
      result.button = blink::WebMouseEvent::Button::kForward;
      break;
    default:
      DCHECK(false);
  }

  result.click_count = clickCount;

  return result;
}

blink::WebMouseEvent CefBrowserPlatformDelegateNativeMac::TranslateWebMoveEvent(
    const CefMouseEvent& mouse_event,
    bool mouseLeave) const {
  blink::WebMouseEvent result;
  TranslateWebMouseEvent(result, mouse_event);

  if (!mouseLeave) {
    result.SetType(blink::WebInputEvent::Type::kMouseMove);
    if (mouse_event.modifiers & CEF_EVENTFLAG_LEFT_MOUSE_BUTTON) {
      result.button = blink::WebMouseEvent::Button::kLeft;
    } else if (mouse_event.modifiers & CEF_EVENTFLAG_MIDDLE_MOUSE_BUTTON) {
      result.button = blink::WebMouseEvent::Button::kMiddle;
    } else if (mouse_event.modifiers & CEF_EVENTFLAG_RIGHT_MOUSE_BUTTON) {
      result.button = blink::WebMouseEvent::Button::kRight;
    } else if (mouse_event.modifiers & CEF_EVENTFLAG_BACK_MOUSE_BUTTON) {
      result.button = blink::WebMouseEvent::Button::kBack;
    } else if (mouse_event.modifiers & CEF_EVENTFLAG_FORWARD_MOUSE_BUTTON) {
      result.button = blink::WebMouseEvent::Button::kForward;
    } else {
      result.button = blink::WebMouseEvent::Button::kNoButton;
    }
  } else {
    result.SetType(blink::WebInputEvent::Type::kMouseLeave);
    result.button = blink::WebMouseEvent::Button::kNoButton;
  }

  result.click_count = 0;

  return result;
}

blink::WebMouseWheelEvent
CefBrowserPlatformDelegateNativeMac::TranslateWebWheelEvent(
    const CefMouseEvent& mouse_event,
    int deltaX,
    int deltaY) const {
  blink::WebMouseWheelEvent result;
  TranslateWebMouseEvent(result, mouse_event);

  result.SetType(blink::WebInputEvent::Type::kMouseWheel);

  static const double scrollbarPixelsPerCocoaTick = 40.0;
  result.delta_x = deltaX;
  result.delta_y = deltaY;
  result.wheel_ticks_x = deltaX / scrollbarPixelsPerCocoaTick;
  result.wheel_ticks_y = deltaY / scrollbarPixelsPerCocoaTick;
  result.delta_units = ui::ScrollGranularity::kScrollByPrecisePixel;

  if (mouse_event.modifiers & CEF_EVENTFLAG_LEFT_MOUSE_BUTTON) {
    result.button = blink::WebMouseEvent::Button::kLeft;
  } else if (mouse_event.modifiers & CEF_EVENTFLAG_MIDDLE_MOUSE_BUTTON) {
    result.button = blink::WebMouseEvent::Button::kMiddle;
  } else if (mouse_event.modifiers & CEF_EVENTFLAG_RIGHT_MOUSE_BUTTON) {
    result.button = blink::WebMouseEvent::Button::kRight;
  } else if (mouse_event.modifiers & CEF_EVENTFLAG_BACK_MOUSE_BUTTON) {
    result.button = blink::WebMouseEvent::Button::kBack;
  } else if (mouse_event.modifiers & CEF_EVENTFLAG_FORWARD_MOUSE_BUTTON) {
    result.button = blink::WebMouseEvent::Button::kForward;
  } else {
    result.button = blink::WebMouseEvent::Button::kNoButton;
  }

  return result;
}

void CefBrowserPlatformDelegateNativeMac::TranslateWebMouseEvent(
    blink::WebMouseEvent& result,
    const CefMouseEvent& mouse_event) const {
  // position
  result.SetPositionInWidget(mouse_event.x, mouse_event.y);

  const gfx::Point& screen_pt = GetScreenPoint(
      gfx::Point(mouse_event.x, mouse_event.y), /*want_dip_coords=*/true);
  result.SetPositionInScreen(screen_pt.x(), screen_pt.y());

  // modifiers
  result.SetModifiers(result.GetModifiers() |
                      TranslateWebEventModifiers(mouse_event.modifiers));

  // timestamp
  result.SetTimeStamp(base::TimeTicks() +
                      base::Seconds(currentEventTimestamp()));

  result.pointer_type = blink::WebPointerProperties::PointerType::kMouse;
}

content::RenderWidgetHostViewMac*
CefBrowserPlatformDelegateNativeMac::GetHostView() const {
  if (!web_contents_) {
    return nullptr;
  }
  return static_cast<content::RenderWidgetHostViewMac*>(
      web_contents_->GetRenderWidgetHostView());
}
