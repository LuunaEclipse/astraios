#include "window_manager.hpp"
#include <spdlog/spdlog.h>
#include <X11/Xutil.h>

using ::std::unique_ptr;

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

    wm_detected_ = false;
    XSetErrorHandler(&WindowManager::OnWMDetected);
    XSelectInput(display_, root_, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(display_, false);
    if(wm_detected_){
        spdlog::error(std::strcat("Detected another window manager on display ", XDisplayString(display_)));
        return;
    }
    XSetErrorHandler(&WindowManager::OnXError);

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

void WindowManager::OnCreateNotify(const XCreateWindowEvent& e){}

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
    spdlog::info("Resize " + e.window + " to " + std::size<int>(e.width,e.height));
}

int WindowManager::OnWMDetected(Display* display, XErrorEvent* e)
{
    CHECK_EQ(static_cast<int>(e->error_code), BadAccess);    
    wm_detected_ = true;
    return 0;
}

int WindowManager::OnXError(Display* display, XErrorEvent* e) {}