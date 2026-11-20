// Copyright 2026 The Chromium Embedded Framework Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "include/wrapper/cef_closure_task.h"
#include "tests/ceftests/test_handler.h"
#include "tests/ceftests/test_util.h"

// Window delegate that lets the test control window close handling.
@interface ClientWindowTestDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, copy) BOOL (^shouldClose)(void);
@end

@implementation ClientWindowTestDelegate

@synthesize shouldClose = shouldClose_;

- (BOOL)windowShouldClose:(NSWindow*)window {
  return self.shouldClose ? self.shouldClose() : YES;
}
@end

// Runs a block via -performSelectorOnMainThread:, after any work that CEF has
// already queued the same way.
@interface ClientWindowTestTask : NSObject
+ (void)runAfterQueuedWork:(void (^)(void))block;
@end

@implementation ClientWindowTestTask {
  void (^block_)(void);
}

+ (void)runAfterQueuedWork:(void (^)(void))block {
  ClientWindowTestTask* task = [[ClientWindowTestTask alloc] init];
  task->block_ = [block copy];
  [task performSelectorOnMainThread:@selector(run)
                         withObject:nil
                      waitUntilDone:NO];
}

- (void)run {
  block_();
}
@end

namespace {

class RetainedHostViewTestHandler : public TestHandler {
 public:
  void RunTest() override {
    SetTestTimeout(10000);
    SetUseViews(false);
    SetUseAlloyStyle(true, true);
    CreateBrowser("about:blank");
  }

  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
    TestHandler::OnAfterCreated(browser);
    // AppKit or an embedder may retain the host view beyond window closure.
    // This must not delay OnBeforeClose until that reference is released.
    host_view_ =
        CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(browser->GetHost()->GetWindowHandle());
    EXPECT_NE(nil, host_view_);
    CefPostTask(TID_UI, base::BindOnce(
                            &RetainedHostViewTestHandler::DestroyTest, this));
  }

  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
    EXPECT_NE(nil, host_view_);
    // Releasing the view after browser destruction must not notify it again.
    host_view_ = nil;
    TestHandler::OnBeforeClose(browser);
  }

  void OnTestTimeout(int timeout_ms, bool treat_as_error) override {
    TestHandler::OnTestTimeout(timeout_ms, treat_as_error);
    // Also permit teardown with the old implementation, which waits for the
    // host view to deallocate before delivering OnBeforeClose.
    host_view_ = nil;
  }

 private:
  NSView* __strong host_view_ = nil;

  IMPLEMENT_REFCOUNTING(RetainedHostViewTestHandler);
};

// Base class for browsers parented to a client-owned NSWindow
// (CefWindowInfo::SetAsChild). The client keeps references to its windows and
// views, so closing a window does not deallocate the CEF host view
// (issue #3810). Subclasses implement StartClose() and VerifyBeforeClose().
class ClientWindowTestHandler : public TestHandler {
 public:
  void RunTest() override {
    SetTestTimeout(10000);
    SetUseViews(false);
    SetUseAlloyStyle(true, true);
    if (!UsesDestroyTest()) {
      SetDestroyTestExpected(false);
    }
    // AppKit objects must be created on the main (UI) thread.
    CefPostTask(TID_UI,
                base::BindOnce(&ClientWindowTestHandler::CreateWindows, this));
  }

  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
    TestHandler::OnAfterCreated(browser);
    host_view_ =
        CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(browser->GetHost()->GetWindowHandle());
    EXPECT_EQ(window_, [host_view_ window]);
    CefPostTask(TID_UI,
                base::BindOnce(&ClientWindowTestHandler::StartClose, this));
  }

  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
    VerifyBeforeClose();
    before_close_count_++;
    // Releasing the views after browser destruction must not notify the
    // browser again.
    ReleaseWindows();
    TestHandler::OnBeforeClose(browser);
  }

  void OnTestTimeout(int timeout_ms, bool treat_as_error) override {
    TestHandler::OnTestTimeout(timeout_ms, treat_as_error);
    // Permit teardown with the old implementation, which waits for the host
    // view to deallocate before delivering OnBeforeClose.
    ReleaseWindows();
  }

  int before_close_count() const { return before_close_count_; }

 protected:
  // Returns false if the client closes the browser without DestroyTest().
  virtual bool UsesDestroyTest() const { return true; }
  // Called before the browser is created, to add delegates or windows.
  virtual void ConfigureWindows() {}
  virtual void StartClose() = 0;
  virtual void VerifyBeforeClose() = 0;

  static NSWindow* CreateNSWindow() {
    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 400, 300)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    window.releasedWhenClosed = NO;
    [window orderFront:nil];
    return window;
  }

  void SetWindowShouldClose(BOOL (^should_close)(void)) {
    window_delegate_ = [[ClientWindowTestDelegate alloc] init];
    window_delegate_.shouldClose = should_close;
    window_.delegate = window_delegate_;
  }

  // Runs |check| after any deferred work that CEF has already queued, so that
  // it can verify that the work did not close or destroy anything.
  void CheckAfterQueuedWork(base::OnceClosure check) {
    __block base::OnceClosure block_check = std::move(check);
    [ClientWindowTestTask runAfterQueuedWork:^{
      std::move(block_check).Run();
    }];
  }

  NSWindow* __strong window_ = nil;
  NSWindow* __strong other_window_ = nil;
  NSView* __strong parent_view_ = nil;
  NSView* __strong host_view_ = nil;

 private:
  void CreateWindows() {
    window_ = CreateNSWindow();
    parent_view_ = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 400, 300)];
    [[window_ contentView] addSubview:parent_view_];
    ConfigureWindows();

    CefWindowInfo window_info;
    window_info.SetAsChild(CAST_NSVIEW_TO_CEF_WINDOW_HANDLE(parent_view_),
                           CefRect(0, 0, 400, 300));
    window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
    CefBrowserHost::CreateBrowser(window_info, this, "about:blank",
                                  CefBrowserSettings(), nullptr, nullptr);
  }

  void ReleaseWindows() {
    if (window_delegate_) {
      window_.delegate = nil;
      window_delegate_.shouldClose = nil;
      window_delegate_ = nil;
    }
    host_view_ = nil;
    parent_view_ = nil;
    window_ = nil;
    other_window_ = nil;
  }

  ClientWindowTestDelegate* __strong window_delegate_ = nil;
  int before_close_count_ = 0;
};

// CloseBrowser(false). CEF closes the client window.
class CloseWithRetainedClientWindowTestHandler
    : public ClientWindowTestHandler {
 protected:
  void StartClose() override { DestroyTest(); }
  void VerifyBeforeClose() override {
    EXPECT_FALSE([window_ isVisible]);
    EXPECT_EQ(window_, [host_view_ window]);
  }

  IMPLEMENT_REFCOUNTING(CloseWithRetainedClientWindowTestHandler);
};

// The view moves to a second window before any close. Closing the first
// window must not destroy the browser; closing the browser closes the second.
class MovedToOtherClientWindowTestHandler : public ClientWindowTestHandler {
 protected:
  void StartClose() override {
    other_window_ = CreateNSWindow();
    [[other_window_ contentView] addSubview:parent_view_];
    [window_ close];
    CheckAfterQueuedWork(base::BindOnce(
        &MovedToOtherClientWindowTestHandler::CloseAfterMove, this));
  }
  void CloseAfterMove() {
    EXPECT_TRUE(GetBrowser());
    got_check_ = true;
    DestroyTest();
  }
  void VerifyBeforeClose() override {
    EXPECT_TRUE(got_check_);
    EXPECT_FALSE([other_window_ isVisible]);
    EXPECT_EQ(other_window_, [host_view_ window]);
  }

 private:
  bool got_check_ = false;

  IMPLEMENT_REFCOUNTING(MovedToOtherClientWindowTestHandler);
};

// The client removes the view, then calls CloseBrowser(false). There is no
// window to close, so the browser is destroyed and the window stays open.
class RemovedBeforeCloseTestHandler : public ClientWindowTestHandler {
 protected:
  void StartClose() override {
    [parent_view_ removeFromSuperview];
    DestroyTest();
  }
  void VerifyBeforeClose() override {
    EXPECT_TRUE([window_ isVisible]);
    EXPECT_EQ(nil, [host_view_ window]);
  }

  IMPLEMENT_REFCOUNTING(RemovedBeforeCloseTestHandler);
};

// The window's windowShouldClose: removes the view and allows the close.
class RemovedInWindowShouldCloseTestHandler : public ClientWindowTestHandler {
 protected:
  void ConfigureWindows() override {
    SetWindowShouldClose(^BOOL {
      [parent_view_ removeFromSuperview];
      return YES;
    });
  }
  void StartClose() override { DestroyTest(); }
  void VerifyBeforeClose() override {
    EXPECT_FALSE([window_ isVisible]);
    EXPECT_EQ(nil, [host_view_ window]);
  }

  IMPLEMENT_REFCOUNTING(RemovedInWindowShouldCloseTestHandler);
};

// The window's windowShouldClose: moves the view to a second window and
// cancels the close. The view left the host window, so the browser is
// destroyed and both windows stay open.
class MovedInWindowShouldCloseTestHandler : public ClientWindowTestHandler {
 protected:
  void ConfigureWindows() override {
    other_window_ = CreateNSWindow();
    SetWindowShouldClose(^BOOL {
      [[other_window_ contentView] addSubview:parent_view_];
      return NO;
    });
  }
  void StartClose() override { DestroyTest(); }
  void VerifyBeforeClose() override {
    EXPECT_TRUE([window_ isVisible]);
    EXPECT_TRUE([other_window_ isVisible]);
    EXPECT_EQ(other_window_, [host_view_ window]);
  }

  IMPLEMENT_REFCOUNTING(MovedInWindowShouldCloseTestHandler);
};

// CloseBrowser(true) with a DoClose() that moves the view to a second window
// and returns true to use a custom close mechanism. CEF must not close either
// window or destroy the browser. The client then closes the second window.
class ReparentedInCustomDoCloseTestHandler : public ClientWindowTestHandler {
 public:
  bool DoClose(CefRefPtr<CefBrowser> browser) override {
    other_window_ = CreateNSWindow();
    [[other_window_ contentView] addSubview:parent_view_];
    CheckAfterQueuedWork(base::BindOnce(
        &ReparentedInCustomDoCloseTestHandler::CompleteCustomClose, this));
    return true;
  }

 protected:
  bool UsesDestroyTest() const override { return false; }
  void StartClose() override {
    GetBrowser()->GetHost()->CloseBrowser(/*force_close=*/true);
  }
  void CompleteCustomClose() {
    EXPECT_TRUE([other_window_ isVisible]);
    EXPECT_TRUE(GetBrowser());
    got_check_ = true;
    [other_window_ close];
  }
  void VerifyBeforeClose() override {
    EXPECT_TRUE(got_check_);
    EXPECT_TRUE([window_ isVisible]);
    EXPECT_FALSE([other_window_ isVisible]);
  }

 private:
  bool got_check_ = false;

  IMPLEMENT_REFCOUNTING(ReparentedInCustomDoCloseTestHandler);
};

// The client closes the window that contains the view, then moves the view
// to a second window. The window close destroys the browser and the second
// window stays open.
class ClosedThenMovedTestHandler : public ClientWindowTestHandler {
 protected:
  bool UsesDestroyTest() const override { return false; }
  void StartClose() override {
    [window_ close];
    other_window_ = CreateNSWindow();
    [[other_window_ contentView] addSubview:parent_view_];
  }
  void VerifyBeforeClose() override {
    EXPECT_FALSE([window_ isVisible]);
    EXPECT_TRUE([other_window_ isVisible]);
    EXPECT_EQ(other_window_, [host_view_ window]);
  }

  IMPLEMENT_REFCOUNTING(ClosedThenMovedTestHandler);
};

template <typename T>
void RunClientWindowTest() {
  CefRefPtr<T> handler = new T();
  handler->ExecuteTest();
  EXPECT_EQ(1, handler->before_close_count());
  ReleaseAndWaitForDestructor(handler);
}

}  // namespace

TEST(BrowserWindowTest, CloseWithRetainedHostView) {
  CefRefPtr<RetainedHostViewTestHandler> handler =
      new RetainedHostViewTestHandler;
  handler->ExecuteTest();
  ReleaseAndWaitForDestructor(handler);
}

TEST(BrowserWindowTest, CloseChildWithRetainedClientWindow) {
  RunClientWindowTest<CloseWithRetainedClientWindowTestHandler>();
}

TEST(BrowserWindowTest, CloseChildMovedToOtherClientWindow) {
  RunClientWindowTest<MovedToOtherClientWindowTestHandler>();
}

TEST(BrowserWindowTest, CloseChildRemovedBeforeClose) {
  RunClientWindowTest<RemovedBeforeCloseTestHandler>();
}

TEST(BrowserWindowTest, CloseChildRemovedInClientWindowShouldClose) {
  RunClientWindowTest<RemovedInWindowShouldCloseTestHandler>();
}

TEST(BrowserWindowTest, CloseChildMovedInClientWindowShouldClose) {
  RunClientWindowTest<MovedInWindowShouldCloseTestHandler>();
}

TEST(BrowserWindowTest, CloseChildReparentedInCustomDoClose) {
  RunClientWindowTest<ReparentedInCustomDoCloseTestHandler>();
}

TEST(BrowserWindowTest, CloseChildClosedThenMoved) {
  RunClientWindowTest<ClosedThenMovedTestHandler>();
}
