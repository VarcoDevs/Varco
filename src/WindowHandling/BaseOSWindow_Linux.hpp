#ifndef VARCO_BASEOSWINDOW_LINUX_HPP
#define VARCO_BASEOSWINDOW_LINUX_HPP

#include <GL/glx.h>
#include <X11/Xlib.h>

#include <Utils/VKeyCodes.hpp>
#include <SkCanvas.h>
#include <SkSurface.h>
#include <chrono>
#include <thread>

namespace varco {

  class BaseOSWindow {
  public:

    BaseOSWindow(int argc, char **argv);

    int show();

    virtual void draw(SkCanvas& canvas) = 0;
    void redraw();
    virtual void onLeftMouseDown(SkScalar x, SkScalar y) = 0;
    virtual void onLeftMouseMove(SkScalar x, SkScalar y) = 0;
    void startMouseCapture();
    virtual void onMouseLeave() = 0;
    virtual void onLeftMouseUp(SkScalar x, SkScalar y) = 0;
    virtual void onKeyDown(VirtualKeycode key) = 0;

  protected:
    int Argc;
    char **Argv;
    int Width, Height;
    SkBitmap Bitmap;
    GLXContext fGLContext;
    
    Display *fDisplay = nullptr;    
    XVisualInfo *fVi  = nullptr; // VisualInfo structure for GL
    Window fWin;
    GC fGc; // Graphic context

  private:

    void mapWindowAndWait();
    void invalidateWindow();
    std::chrono::time_point<std::chrono::system_clock> lastExposeEventTime;
    std::unique_ptr<std::thread> redrawThread;
    bool fNeedDraw = true;
    bool wndProc(XEvent *evt); // Returns false to exit the loop
    void resize(int width, int height);
    void paint();
  };

}

#endif // VARCO_BASEOSWINDOW_LINUX_HPP
