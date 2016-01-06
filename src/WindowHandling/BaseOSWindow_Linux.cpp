#include <WindowHandling/BaseOSWindow_Linux.hpp>

#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "SkEvent.h"

namespace varco {

  // Events we'll be listening for
  const long EVENT_MASK = StructureNotifyMask|ButtonPressMask|
                          ButtonReleaseMask|ExposureMask|PointerMotionMask|
                          KeyPressMask|KeyReleaseMask;

  BaseOSWindow::BaseOSWindow(int argc, char **argv)
    : Argc(argc), Argv(argv), Width(545), Height(355)
  {
    // Connect to the X server
    fDisplay = XOpenDisplay(nullptr);
    if (fDisplay == nullptr) {
      SkDebugf("Could not open a connection to an X Display");
      return;
    }
    
    GLint att[] = { // Create a window which supports GL
      GLX_RGBA,
      GLX_DEPTH_SIZE, 24,
      GLX_DOUBLEBUFFER,
      GLX_STENCIL_SIZE, 8,
      None
    };
    
    fVi = glXChooseVisual(fDisplay, DefaultScreen(fDisplay), att);
    
    Colormap colorMap = XCreateColormap(fDisplay,
                                        RootWindow(fDisplay, fVi->screen),
                                        fVi->visual,
                                        AllocNone);
    XSetWindowAttributes swa;
    swa.colormap = colorMap;
    swa.event_mask = EVENT_MASK;
    fWin = XCreateWindow(fDisplay,
                         RootWindow(fDisplay, fVi->screen),
                         0, 0, // x, y
                         Width, Height,
                         0, // border width
                         fVi->depth,
                         InputOutput,
                         fVi->visual,
                         CWEventMask | CWColormap,
                         &swa);

    mapWindowAndWait();
    fGc = XCreateGC(fDisplay, fWin, 0, nullptr);

    this->resize(Width, Height);
  }

  void BaseOSWindow::mapWindowAndWait() {
    SkASSERT(fDisplay);
    // Map the window on screen (i.e. make it visible)
    XMapWindow(fDisplay, fWin);

    // Ask to be notified of mapping accomplished
    long eventMask = StructureNotifyMask;
    XSelectInput(fDisplay, fWin, eventMask);

    // Wait for the window mapping confirmation
    XEvent evt;
    do {
      XNextEvent(fDisplay, &evt);
    } while(evt.type != MapNotify);
  }

  void BaseOSWindow::resize(int width, int height) {
    if (width != Bitmap.width() || height != Bitmap.height()) {
      Bitmap.allocPixels(SkImageInfo::Make(width, height,
                                           kN32_SkColorType,
                                           kPremul_SkAlphaType));
      this->Width = width;
      this->Height = height;
    }
  }

  static SkMSec gTimerDelay;

  static bool MyXNextEventWithDelay(Display* dsp, XEvent* evt) {
    // Check for pending events before entering the select loop. There might
    // be events in the in-memory queue but not processed yet.
    if (XPending(dsp)) {
        XNextEvent(dsp, evt);
        return true;
      }

    SkMSec ms = gTimerDelay;
    if (ms > 0) {
        int x11_fd = ConnectionNumber(dsp);
        fd_set input_fds;
        FD_ZERO(&input_fds);
        FD_SET(x11_fd, &input_fds);

        timeval tv;
        tv.tv_sec = ms / 1000;              // seconds
        tv.tv_usec = (ms % 1000) * 1000;    // microseconds

        if (!select(x11_fd + 1, &input_fds, nullptr, nullptr, &tv)) {
            if (!XPending(dsp)) {
                return false;
              }
          }
      }
    XNextEvent(dsp, evt);
    return true;
  }

  static Atom wm_delete_window_message;

  bool BaseOSWindow::wndProc(XEvent *evt) {

    switch (evt->type) {
      case Expose: {
        // TODO PAINT
      } break;
      case ConfigureNotify: {
        this->resize(evt->xconfigure.width, evt->xconfigure.height);
      } break;
      case ClientMessage: {
        if ((Atom)evt->xclient.data.l[0] == wm_delete_window_message)
          return false;
      } break;
        // fallthrough
      default:
        break;
      }
    return true; // Handled
  }

  int BaseOSWindow::show() {
    if (fDisplay == nullptr) {
      return 1; // Error
    }

    wm_delete_window_message = XInternAtom(fDisplay, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(fDisplay, fWin, &wm_delete_window_message, 1);

    XSelectInput(fDisplay, fWin, EVENT_MASK);

    while(1) {
      XEvent evt;
      XNextEvent(fDisplay, &evt);

      auto ret = wndProc(&evt);

      if(ret == false) // Exit requested
        break;
    }
  }

} // namespace varco
