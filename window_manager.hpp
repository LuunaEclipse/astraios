extern "C" {
#include <X11/Xlib.h>
}
#include <memory>

class WindowManager {
    public: static ::std::unique_ptr<WindowManager> Create();
    ~WindowManager();
    void Run();

    private: WindowManager(Display* display);
    Display* display_;
    const Window root_;

    void OnCreateNotify(const XCreateWindowEvent& e);
    void OnDestroyNotify(const XDestroyWindowEvent& e);
    void OnReparentNotify(const XReparentEvent& e);
    void OnMapNotify(const XMapEvent& e);
    void OnUnmapNotify(const XUnmapEvent& e);
    void OnConfigureNotify(const XConfigureEvent& e);
    void OnMapRequest(const XMapRequestEvent& e);
    void OnButtonPress(const XButtonPressedEvent& e);
    void OnButtonRelease(const XButtonReleasedEvent& e);
    void OnMotionNotify(const XMotionEvent& e);
    void OnKeyPress(const XKeyPressedEvent& e);
    void OnKeyRelease(const XKeyReleasedEvent& e);
    void OnConfigureRequest(const XConfigureRequestEvent& e);


    static int OnXError(Display* display, XErrorEvent* e);
    static int OnWMDetected(Display* display, XErrorEvent* e);
    static bool wm_detected_;
};