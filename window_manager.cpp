#include "window_manager.hpp"
#include <spdlog/spdlog.h>
#include <X11/Xutil.h>
#include <iostream>

using ::std::unique_ptr;

bool WindowManager::wm_detected_;
Window esde;

unique_ptr<WindowManager> WindowManager::Create()
{
    Display* display = XOpenDisplay(nullptr);
    if(display == nullptr)
    {
        spdlog::error("Failed to open X display");
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

WindowManager::WindowManager(Display* display): display_(CHECK_NOTNULL(display)), root_(DefaultRootWindow(display_)) {}

WindowManager::~WindowManager()
{
    XCloseDisplay(display_);
}

void WindowManager::Run()
{
    const char *type[37] = {0};
    type[2] = "KeyPress";
    type[3] = "KeyRelease";
    type[4] = "ButtonPress";
    type[5] = "ButtonRelease";
    type[6] = "MotionNotify";
    type[7] = "EnterNotify";
    type[8] = "LeaveNotify";
    type[9] = "FocusIn";
    type[10] = "FocusOut";
    type[11] = "KeymapNotify";
    type[12] = "Expose";
    type[13] = "GraphicsExpose";
    type[14] = "NoExpose";
    type[15] = "VisibilityNotify";
    type[16] = "CreateNotify";  
    type[17] = "DestroyNotify";
    type[18] = "UnmapNotify";
    type[19] = "MapNotify";
    type[20] = "MapRequest";
    type[21] = "ReparentNotify";
    type[22] = "ConfigureNotify";
    type[23] = "ConfigureRequest";
    type[24] = "GravityNotify";
    type[25] = "ResizeRequest";
    type[26] = "CirculateNotify";
    type[27] = "CirculateRequest";
    type[28] = "PropertyNotify";
    type[29] = "SelectionClear";
    type[30] = "SelectionRequest";
    type[31] = "SelectionNotify";
    type[32] = "ColormapNotify";
    type[33] = "ClientMessage";
    type[34] = "MappingNotify";
    type[35] = "GenericEvent";
    type[36] = "LASTEvent";

    WindowManager::wm_detected_ = false;
    XSetErrorHandler(&WindowManager::OnWMDetected);
    XSelectInput(display_, root_, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(display_, false);
    if(WindowManager::wm_detected_){
        spdlog::error(std::strcat("Detected another window manager on display ", XDisplayString(display_)));
        return;
    }
    XSetErrorHandler(&WindowManager::OnXError);

    XGrabServer(display_);
    Window returned_root, returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    if(XQueryTree(display_, root_, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows) == 0) std::exit(EXIT_FAILURE);
    CHECK_EQ(returned_root, root_);
    Window esde = top_level_windows[0];
    spdlog::info(esde);
    XUngrabServer(display_);

    std::exit(EXIT_SUCCESS);

    XResizeWindow(display_,esde,300,300);

    for (;;) {
        XEvent e;
        XNextEvent(display_, &e);
        spdlog::info("Recieved event: " + std::to_string(e.type));

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
                // Skip any already pending motion events.
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
                spdlog::warn("Ignored event");
        }
    }
}

void WindowManager::OnReparentNotify(const XReparentEvent& e) {}
void WindowManager::OnMapNotify(const XMapEvent& e) {}
void WindowManager::OnConfigureNotify(const XConfigureEvent& e) {}
void WindowManager::OnButtonPress(const XButtonPressedEvent& e) {}
void WindowManager::OnButtonRelease(const XButtonReleasedEvent& e) {}
void WindowManager::OnMotionNotify(const XMotionEvent& e) {}
void WindowManager::OnKeyPress(const XKeyPressedEvent& e) {}
void WindowManager::OnKeyRelease(const XKeyReleasedEvent& e) {}
void WindowManager::OnCreateNotify(const XCreateWindowEvent& e){}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e)
{
    XSelectInput(display_,esde,SubstructureRedirectMask | SubstructureNotifyMask);
    XMapWindow(display_, esde);
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
    XConfigureWindow(display_, e.window, e.value_mask, &changes);
}

void WindowManager::OnMapRequest(const XMapRequestEvent& e) 
{
    Window w = e.window;
    XSelectInput(display_,w,SubstructureRedirectMask | SubstructureNotifyMask);
    XAddToSaveSet(display_,w);
    XGrabKey(
        display_,
        XKeysymToKeycode(display_, XK_F4),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync);
    //   d. Switch windows with alt + tab.
    XGrabKey(
        display_,
        XKeysymToKeycode(display_, XK_Tab),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync);
    XMapWindow(display_, w);
}

void WindowManager::OnUnmapNotify(const XUnmapEvent& e)
{
    XSelectInput(display_,esde,SubstructureRedirectMask | SubstructureNotifyMask);
    XDestroyWindow(display_,e.window);
    XRemoveFromSaveSet(display_, e.window);
}

int WindowManager::OnWMDetected(Display* display, XErrorEvent* e)
{
    CHECK_EQ(static_cast<int>(e->error_code), BadAccess);
    WindowManager::wm_detected_ = true;
    return 0;
}

int WindowManager::OnXError(Display* display, XErrorEvent* e) {}