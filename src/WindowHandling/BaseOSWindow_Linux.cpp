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

  static Atom wm_delete_window_message;

  namespace {

    void invalidateWindow(Display *display, Window window) { // Fire a redraw() event
      XEvent exppp;
      memset(&exppp, 0, sizeof(exppp));
      exppp.type = Expose;
      exppp.xexpose.window = window;
      XSendEvent(display, window, False, ExposureMask, &exppp);
    }

  }

  bool BaseOSWindow::wndProc(XEvent *evt) {

    switch (evt->type) {

      case Expose: {

        if (!(Width > 0 && Height > 0))
          return 1; // Nonsense painting a 0 area

        // Call the OS-independent draw function
        SkCanvas canvas(this->Bitmap);
        this->draw(canvas);

        // Finally do the painting after the drawing is done
        this->paint();

      } break;

      case ConfigureNotify: {
        this->resize(evt->xconfigure.width, evt->xconfigure.height);

        invalidateWindow(this->fDisplay, this->fWin);
      } break;

      case ClientMessage: {
        if ((Atom)evt->xclient.data.l[0] == wm_delete_window_message)
          return false;
      } break;

      case ButtonPress: { // Mouse input

        switch (evt->xbutton.button) {

          case Button1: {
            int x = evt->xbutton.x;
            int y = evt->xbutton.y;

            bool ret = this->onMouseDown(x, y);
            if (ret)
              invalidateWindow(this->fDisplay, this->fWin);
          } break;

          default:
            break;
        }
      } break;

      default: // fallthrough
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

  void BaseOSWindow::paint() {
    if (fDisplay == nullptr)
      return; // No X display

    // Draw the bitmap to the screen.
    int width = Bitmap.width();
    int height = Bitmap.height();

    // Convert the bitmap to XImage
    XImage image;
    sk_bzero(&image, sizeof(image));

    int bitsPerPixel = Bitmap.bytesPerPixel() * 8;
    image.width = Bitmap.width();
    image.height = Bitmap.height();
    image.format = ZPixmap;
    image.data = (char*) Bitmap.getPixels();
    image.byte_order = LSBFirst;
    image.bitmap_unit = bitsPerPixel;
    image.bitmap_bit_order = LSBFirst;
    image.bitmap_pad = bitsPerPixel;
    image.depth = 24;
    image.bytes_per_line = Bitmap.rowBytes() - Bitmap.width() * 4;
    image.bits_per_pixel = bitsPerPixel;

    auto res = XInitImage(&image);
    if(res == false)
      return;

    XPutImage(fDisplay, fWin, fGc, &image,
              0, 0,     // src x,y
              0, 0,     // dst x,y
              width, height);
  }

} // namespace varco
