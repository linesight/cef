// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <X11/Xlib.h>
#include <gdk/gdkwayland.h>
#include <glib-unix.h>
#include <gtk/gtk.h>
#undef Success     // Definition conflicts with cef_message_router.h
#undef RootWindow  // Definition conflicts with root_window.h

#include <locale.h>
#include <stdlib.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "include/base/cef_logging.h"
#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"
#include "tests/cefclient/browser/main_context_impl.h"
#include "tests/cefclient/browser/main_message_loop_multithreaded_gtk.h"
#include "tests/cefclient/browser/test_runner.h"
#include "tests/shared/browser/client_app_browser.h"
#include "tests/shared/browser/main_message_loop_external_pump.h"
#include "tests/shared/browser/main_message_loop_std.h"
#include "tests/shared/common/client_app_other.h"
#include "tests/shared/common/client_switches.h"
#include "tests/shared/renderer/client_app_renderer.h"

namespace client {
namespace {

int XErrorHandlerImpl(Display* display, XErrorEvent* event) {
  LOG(WARNING) << "X error received: " << "type " << event->type << ", "
               << "serial " << event->serial << ", " << "error_code "
               << static_cast<int>(event->error_code) << ", " << "request_code "
               << static_cast<int>(event->request_code) << ", " << "minor_code "
               << static_cast<int>(event->minor_code);
  return 0;
}

int XIOErrorHandlerImpl(Display* display) {
  return 0;
}

int WaylandEmbedError(const char* message) {
  LOG(ERROR) << "cefclient: --wayland-embed " << message;
  return EXIT_FAILURE;
}

void TerminationSignalHandler(int signatl) {
  LOG(ERROR) << "Received termination signal: " << signatl;
  MainContext::Get()->GetRootWindowManager()->CloseAllWindows(true);
}

gboolean WaylandTerminationSignalHandler(gpointer data) {
  const int signatl = GPOINTER_TO_INT(data);
  LOG(ERROR) << "Received termination signal: " << signatl;
  MainContext::Get()->GetRootWindowManager()->CloseAllWindows(true);
  return G_SOURCE_CONTINUE;
}

NO_STACK_PROTECTOR
int RunMain(int argc, char* argv[]) {
  // Create a copy of |argv| on Linux because Chromium mangles the value
  // internally (see issue #620).
  CefScopedArgArray scoped_arg_array(argc, argv);
  char** argv_copy = scoped_arg_array.array();

  CefMainArgs main_args(argc, argv);

  // Parse command-line arguments.
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromArgv(argc, argv);

  // Create a ClientApp of the correct type.
  CefRefPtr<CefApp> app;
  ClientApp::ProcessType process_type = ClientApp::GetProcessType(command_line);
  if (process_type == ClientApp::BrowserProcess) {
    app = new ClientAppBrowser();
  } else if (process_type == ClientApp::RendererProcess ||
             process_type == ClientApp::ZygoteProcess) {
    // On Linux the zygote process is used to spawn other process types. Since
    // we don't know what type of process it will be give it the renderer
    // client.
    app = new ClientAppRenderer();
  } else if (process_type == ClientApp::OtherProcess) {
    app = new ClientAppOther();
  }

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app, nullptr);
  if (exit_code >= 0) {
    return exit_code;
  }

  // Create the main context object.
  auto context = std::make_unique<MainContextImpl>(command_line, true);

  CefSettings settings;

// When generating projects with CMake the CEF_USE_SANDBOX value will be defined
// automatically. Pass -DUSE_SANDBOX=OFF to the CMake command-line to disable
// use of the sandbox.
#if !defined(CEF_USE_SANDBOX)
  settings.no_sandbox = true;
#endif

  // Populate the settings based on command line arguments.
  context->PopulateSettings(&settings);

  // Wayland window-embedding harness (CEF issue #2804). When --wayland-embed is
  // passed, open GDK's Wayland display BEFORE CEF so that CEF can share the
  // wl_display connection. Do not initialize GTK here: Chromium initializes it
  // during CefInitialize(), after establishing the same locale policy below.
  // Without the switch, the default X11/XWayland path is unchanged.
  const bool use_wayland = command_line->HasSwitch(switches::kWaylandEmbed);
  if (use_wayland) {
#if !CEF_API_ADDED(CEF_EXPERIMENTAL)
    return WaylandEmbedError("requires the experimental CEF API");
#else
    if (!command_line->HasSwitch(switches::kUseNative) ||
        command_line->HasSwitch(switches::kUseViews)) {
      return WaylandEmbedError(
          "requires --use-native and cannot be combined with --use-views");
    }
    if (!command_line->HasSwitch(switches::kUseAlloyStyle)) {
      return WaylandEmbedError("requires --use-alloy-style");
    }
    if (command_line->HasSwitch(switches::kOzonePlatform) &&
        command_line->GetSwitchValue(switches::kOzonePlatform).ToString() !=
            "wayland") {
      return WaylandEmbedError(
          "cannot be combined with a non-Wayland --ozone-platform value");
    }
    if (command_line->HasSwitch("ui-toolkit") &&
        command_line->GetSwitchValue("ui-toolkit").ToString() != "gtk") {
      return WaylandEmbedError(
          "cannot be combined with a non-GTK --ui-toolkit value");
    }
    if (settings.windowless_rendering_enabled ||
        command_line->HasSwitch(switches::kOffScreenRenderingEnabled)) {
      return WaylandEmbedError(
          "cannot be combined with --off-screen-rendering-enabled; OSR is "
          "windowless and does not exercise Wayland subsurface embedding");
    }
    if (command_line->HasSwitch(switches::kMultiThreadedMessageLoop)) {
      return WaylandEmbedError(
          "cannot be combined with --multi-threaded-message-loop; the "
          "embedding harness requires the standard single-threaded loop");
    }
    if (command_line->HasSwitch(switches::kExternalMessagePump)) {
      return WaylandEmbedError(
          "cannot be combined with --external-message-pump; the embedding "
          "harness requires the standard single-threaded loop");
    }
    if (!settings.no_sandbox && !command_line->HasSwitch("no-sandbox")) {
      return WaylandEmbedError(
          "requires a disabled sandbox (pass --no-sandbox or build cefclient "
          "without sandbox support)");
    }

    // Match Chromium's Linux locale policy before any GTK initialization.
    // gtk_disable_setlocale() must run before gtk_init_check(); otherwise GTK
    // can overwrite LC_NUMERIC and Chromium will also emit a warning when it
    // later tries to establish this policy itself.
    if (!setlocale(LC_ALL, "") || !setlocale(LC_NUMERIC, "C")) {
      return WaylandEmbedError("failed to initialize the process locale");
    }
    gtk_disable_setlocale();

    gdk_set_allowed_backends("wayland");
    if (!gdk_init_check(&argc, &argv_copy)) {
      return WaylandEmbedError("could not initialize a GDK Wayland display");
    }

    GdkDisplay* gdk_display = gdk_display_get_default();
    if (!gdk_display || !GDK_IS_WAYLAND_DISPLAY(gdk_display)) {
      return WaylandEmbedError("requires a Wayland session");
    }

    settings.wayland_display = gdk_wayland_display_get_wl_display(gdk_display);
    if (!settings.wayland_display) {
      return WaylandEmbedError("could not obtain GDK's wl_display");
    }

    LOG(INFO) << "cefclient: sharing GDK's Wayland display with CEF; popups, "
                 "picture-in-picture and DevTools are disabled";
    LOG(WARNING) << "cefclient: --wayland-embed is rendering/resize-only; "
                    "mouse/scroll, keyboard, touch, IME and drag-and-drop "
                    "input are not forwarded";
#endif
  }

  // Create the main message loop object.
  std::unique_ptr<MainMessageLoop> message_loop;
  if (settings.multi_threaded_message_loop) {
    message_loop.reset(new MainMessageLoopMultithreadedGtk);
  } else if (settings.external_message_pump) {
    message_loop = MainMessageLoopExternalPump::Create();
  } else {
    message_loop.reset(new MainMessageLoopStd);
  }

  // Initialize the CEF browser process. May return false if initialization
  // fails or if early exit is desired (for example, due to process singleton
  // relaunch behavior).
  if (!context->Initialize(main_args, settings, app, nullptr)) {
    return CefGetExitCode();
  }

  if (!use_wayland) {
    // Force Gtk to use Xwayland (in case a Wayland compositor is being used).
    gdk_set_allowed_backends("x11");

    // The Chromium sandbox requires that there only be a single thread during
    // initialization. Therefore initialize GTK after CEF.
    gtk_init(&argc, &argv_copy);
  }

  // Install xlib error handlers so that the application won't be terminated
  // on non-fatal errors. Must be done after initializing GTK.
  XSetErrorHandler(XErrorHandlerImpl);
  XSetIOErrorHandler(XIOErrorHandlerImpl);

  // Install signal handlers so we clean up after ourselves. The Wayland
  // embedding path must keep its external GTK parent alive until browser
  // teardown completes. Dispatch those signals through GLib's main context so
  // repeated signals cannot concurrently access GTK/CEF state from arbitrary
  // signal-recipient threads.
  if (use_wayland) {
    g_unix_signal_add(SIGINT, WaylandTerminationSignalHandler,
                      GINT_TO_POINTER(SIGINT));
    g_unix_signal_add(SIGTERM, WaylandTerminationSignalHandler,
                      GINT_TO_POINTER(SIGTERM));
  } else {
    signal(SIGINT, TerminationSignalHandler);
    signal(SIGTERM, TerminationSignalHandler);
  }

  // Register scheme handlers.
  test_runner::RegisterSchemeHandlers();

  auto window_config = std::make_unique<RootWindowConfig>();
  window_config->always_on_top =
      command_line->HasSwitch(switches::kAlwaysOnTop);
  window_config->with_osr =
      settings.windowless_rendering_enabled ? true : false;

  // Create the first window.
  context->GetRootWindowManager()->CreateRootWindow(std::move(window_config));

  // Run the message loop. This will block until Quit() is called.
  int result = message_loop->Run();

  // Shut down CEF.
  context->Shutdown();

  // Release objects in reverse order of creation.
  message_loop.reset();
  context.reset();

  return result;
}

}  // namespace
}  // namespace client

// Program entry point function.
NO_STACK_PROTECTOR
int main(int argc, char* argv[]) {
  return client::RunMain(argc, argv);
}
