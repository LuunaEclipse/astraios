#include "window_manager.hpp"
extern "C" {
#include <X11/Xutil.h>
}
#include <cstring>
#include <algorithm>
#include <spdlog/spdlog.h>
#include "util.hpp"
#include <SDL2/SDL.h>

using ::std::max;
using ::std::mutex;
using ::std::string;
using ::std::unique_ptr;

bool WindowManager::wm_detected_;
mutex WindowManager::wm_detected_mutex_;
bool isfirst_ = true;
Window esde_;

unique_ptr<WindowManager> WindowManager::Create(const string& display_str) {
  const char* display_c_str =
        display_str.empty() ? nullptr : display_str.c_str();
  Display* display = XOpenDisplay(display_c_str);
  if (display == nullptr) {
    return nullptr;
  }
  return unique_ptr<WindowManager>(new WindowManager(display));
}

Display* CHECK_NOTNULL(Display* display){
    if(display == NULL) std::exit(EXIT_FAILURE);
    return display;
}

void CHECK_EQ(int con1, int con2){
    if(con1 != con2) std::exit(EXIT_FAILURE);
    return;
}

WindowManager::WindowManager(Display* display)
    : display_(CHECK_NOTNULL(display)),
      root_(DefaultRootWindow(display_)),
      WM_PROTOCOLS(XInternAtom(display_, "WM_PROTOCOLS", false)),
      WM_DELETE_WINDOW(XInternAtom(display_, "WM_DELETE_WINDOW", false)) {
}

WindowManager::~WindowManager() {
  XCloseDisplay(display_);
}

void WindowManager::Run() {
  {
    ::std::lock_guard<mutex> lock(wm_detected_mutex_);

    wm_detected_ = false;
    XSetErrorHandler(&WindowManager::OnWMDetected);
    XSelectInput(
        display_,
        root_,
        SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(display_, false);
    if (wm_detected_) {
      return;
    }
  }
  XSetErrorHandler(&WindowManager::OnXError);
  XGrabServer(display_);
  Window returned_root, returned_parent;
  Window* top_level_windows;
  unsigned int num_top_level_windows;
  XQueryTree(
      display_,
      root_,
      &returned_root,
      &returned_parent,
      &top_level_windows,
      &num_top_level_windows);
  CHECK_EQ(returned_root, root_);
  for (unsigned int i = 0; i < num_top_level_windows; ++i) {
    Frame(top_level_windows[i], true);
  }
  XFree(top_level_windows);
  XUngrabServer(display_);

  for (;;) {
    XEvent e;
    XNextEvent(display_, &e);
    spdlog::info("Recieved event: " + ToString(e));

    switch (e.type) {
      case CreateNotify:
        OnCreateNotify(e.xcreatewindow);
        break;
      case DestroyNotify:
        OnDestroyNotify(e.xdestroywindow);
        break;
      case ReparentNotify:
        OnReparentNotify(e.xreparent);
        break;
      case MapNotify:
        OnMapNotify(e.xmap);
        break;
      case UnmapNotify:
        OnUnmapNotify(e.xunmap);
        break;
      case ConfigureNotify:
        OnConfigureNotify(e.xconfigure);
        break;
      case MapRequest:
        OnMapRequest(e.xmaprequest);
        break;
      case ConfigureRequest:
        OnConfigureRequest(e.xconfigurerequest);
        break;
      case ButtonPress:
        OnButtonPress(e.xbutton);
        break;
      case ButtonRelease:
        OnButtonRelease(e.xbutton);
        break;
      case MotionNotify:
        while (XCheckTypedWindowEvent(
            display_, e.xmotion.window, MotionNotify, &e)) {}
        OnMotionNotify(e.xmotion);
        break;
      case KeyPress:
        OnKeyPress(e.xkey);
        break;
      case KeyRelease:
        OnKeyRelease(e.xkey);
        break;
      default:
        spdlog::warn("Unparseable XEvent. Doing nothing...");
    }
  }
}

Window WindowManager::Frame(Window w, bool was_created_before_window_manager) {
    const unsigned int BORDER_WIDTH = 3;
    const unsigned long BORDER_COLOR = 0xffffff;
    const unsigned long BG_COLOR = 0x000000;

    XWindowAttributes x_window_attrs;
    XGetWindowAttributes(display_, w, &x_window_attrs);

    if (was_created_before_window_manager) {
      if (x_window_attrs.override_redirect ||
          x_window_attrs.map_state != IsViewable) {
        return NULL;
      }
    }

    Window frame;

    if(isfirst_)
    {
      isfirst_ = false;
      esde_ = w;
      XTextProperty prop;
      XGetWMName(display_, esde_, &prop);
      spdlog::info("LAUNCHER FOUND:");
      spdlog::info(reinterpret_cast<char*>(prop.value));
    }

    frame = XCreateSimpleWindow(
        display_,
        root_,
        x_window_attrs.x,
        x_window_attrs.y,
        x_window_attrs.width,
        x_window_attrs.height,
        BORDER_WIDTH,
        BORDER_COLOR,
        BG_COLOR);

    XAddToSaveSet(display_, w);
    XReparentWindow(
        display_,
        w,
        frame,
        0, 0);
    XMapWindow(display_, frame);
    clients_[w] = frame;
    XGrabButton(
        display_,
        Button1,
        Mod1Mask,
        w,
        false,
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None);
    XGrabButton(
        display_,
        Button3,
        Mod1Mask,
        w,
        false,
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None);
    XGrabKey(
        display_,
        XKeysymToKeycode(display_, XK_F8),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync);
    XGrabKey(
        display_,
        XKeysymToKeycode(display_, XK_F9),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync);
    return frame;
}

void WindowManager::Unframe(Window w) {
  const Window frame = clients_[w];
  XUnmapWindow(display_, frame);
  XReparentWindow(
      display_,
      w,
      root_,
      0, 0);
  XRemoveFromSaveSet(display_, w);
  XDestroyWindow(display_, frame);
  clients_.erase(w);
}

void WindowManager::OnCreateNotify(const XCreateWindowEvent& e)
{
}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e) {}

void WindowManager::OnReparentNotify(const XReparentEvent& e) {}

void WindowManager::OnMapNotify(const XMapEvent& e) {}

void WindowManager::OnUnmapNotify(const XUnmapEvent& e) {
  /*
  if (!clients_.count(e.window)) {
    return;
  }
  if (e.event == root_) {
    return;
  }
  */
  XSelectInput(
      display_,
      esde_,
      SubstructureRedirectMask | SubstructureNotifyMask);
  XSetInputFocus(display_, esde_, RevertToPointerRoot, CurrentTime);
  XUnmapWindow(display_, e.window);
  XRemoveFromSaveSet(display_, e.window);
  XDestroyWindow(display_, e.window);
}

void WindowManager::OnConfigureNotify(const XConfigureEvent& e) {}

void WindowManager::OnMapRequest(const XMapRequestEvent& e) {
  if(isfirst_)
    {
      isfirst_ = false;
      esde_ = e.window;
      XTextProperty prop;
      XGetWMName(display_, esde_, &prop);
      spdlog::info("LAUNCHER FOUND:");
      spdlog::info(reinterpret_cast<char*>(prop.value));
    }
  // Window frame = Frame(e.window, false);
  XSelectInput(
      display_,
      e.window,
      SubstructureRedirectMask | SubstructureNotifyMask);
  XMapWindow(display_, e.window);
  XSync(display_, false);
  XSetInputFocus(display_, e.window, RevertToPointerRoot, CurrentTime);
  Atom wm_state   = XInternAtom (display_, "_NET_WM_STATE", true );
  Atom wm_fullscreen = XInternAtom (display_, "_NET_WM_STATE_FULLSCREEN", true );
  XChangeProperty(display_, e.window, wm_state, 0, 32, PropModeReplace, (unsigned char *)&wm_fullscreen, 1);
}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent& e) {
  XWindowChanges changes;
  changes.x = e.x;
  changes.y = e.y;
  changes.width = e.width;
  changes.height = e.height;
  changes.border_width = e.border_width;
  changes.sibling = e.above;
  changes.stack_mode = e.detail;
  if (clients_.count(e.window)) {
    const Window frame = clients_[e.window];
    XConfigureWindow(display_, frame, e.value_mask, &changes);
  }
  XConfigureWindow(display_, e.window, e.value_mask, &changes);
}

void WindowManager::OnButtonPress(const XButtonEvent& e) {
  const Window frame = clients_[e.window];

  drag_start_pos_ = Position<int>(e.x_root, e.y_root);

  Window returned_root;
  int x, y;
  unsigned width, height, border_width, depth;
  XGetGeometry(
      display_,
      frame,
      &returned_root,
      &x, &y,
      &width, &height,
      &border_width,
      &depth);
  drag_start_frame_pos_ = Position<int>(x, y);
  drag_start_frame_size_ = Size<int>(width, height);

  // 3. Raise clicked window to top.
  XSelectInput(
      display_,
      frame,
      SubstructureRedirectMask | SubstructureNotifyMask);
  XRaiseWindow(display_, frame);
}

void WindowManager::OnButtonRelease(const XButtonEvent& e) {}

void WindowManager::OnMotionNotify(const XMotionEvent& e) {
  const Window frame = clients_[e.window];
  const Position<int> drag_pos(e.x_root, e.y_root);
  const Vector2D<int> delta = drag_pos - drag_start_pos_;

  if (e.state & Button1Mask ) {
    // alt + left button: Move window.
    const Position<int> dest_frame_pos = drag_start_frame_pos_ + delta;
    XMoveWindow(
        display_,
        frame,
        dest_frame_pos.x, dest_frame_pos.y);
  } else if (e.state & Button3Mask) {
    // alt + right button: Resize window.
    // Window dimensions cannot be negative.
    const Vector2D<int> size_delta(
        max(delta.x, -drag_start_frame_size_.width),
        max(delta.y, -drag_start_frame_size_.height));
    const Size<int> dest_frame_size = drag_start_frame_size_ + size_delta;
    // 1. Resize frame.
    XResizeWindow(
        display_,
        frame,
        dest_frame_size.width, dest_frame_size.height);
    // 2. Resize client window.
    XResizeWindow(
        display_,
        e.window,
        dest_frame_size.width, dest_frame_size.height);
  }
}

void WindowManager::OnKeyPress(const XKeyEvent& e) {
  if ((e.state & Mod1Mask) &&
      (e.keycode == XKeysymToKeycode(display_, XK_F8))) {
    Atom* supported_protocols;
    int num_supported_protocols;
    if (XGetWMProtocols(display_,
                        e.window,
                        &supported_protocols,
                        &num_supported_protocols) &&
        (::std::find(supported_protocols,
                     supported_protocols + num_supported_protocols,
                     WM_DELETE_WINDOW) !=
         supported_protocols + num_supported_protocols)) {
      XEvent msg;
      memset(&msg, 0, sizeof(msg));
      msg.xclient.type = ClientMessage;
      msg.xclient.message_type = WM_PROTOCOLS;
      msg.xclient.window = e.window;
      msg.xclient.format = 32;
      msg.xclient.data.l[0] = WM_DELETE_WINDOW;
      XSendEvent(display_, e.window, false, 0, &msg);
    } else {
      XKillClient(display_, e.window);
    }
  } else if ((e.state & Mod1Mask) &&
             (e.keycode == XKeysymToKeycode(display_, XK_F9))) {
    std::exit(EXIT_SUCCESS);
  }
}

void WindowManager::OnKeyRelease(const XKeyEvent& e) {}

int WindowManager::OnXError(Display* display, XErrorEvent* e) {
  const int MAX_ERROR_TEXT_LENGTH = 1024;
  char error_text[MAX_ERROR_TEXT_LENGTH];
  XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
  return 0;
}

int WindowManager::OnWMDetected(Display* display, XErrorEvent* e) {
  CHECK_EQ(static_cast<int>(e->error_code), BadAccess);
  wm_detected_ = true;
  return 0;
}
