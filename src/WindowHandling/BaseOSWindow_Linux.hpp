#ifndef VARCO_BASEOSWINDOW_LINUX_HPP
#define VARCO_BASEOSWINDOW_LINUX_HPP

#include <GL/glx.h>
#include <X11/Xlib.h>

#include "SkCanvas.h"
#include "SkSurface.h"

namespace varco {

  class BaseOSWindow {
  public:

    BaseOSWindow(int argc, char **argv);

    int show();

    virtual void draw(SkCanvas& canvas) = 0;
    virtual bool onMouseDown(SkScalar x, SkScalar y) = 0;

  protected:
    int Argc;
    char **Argv;
    int Width, Height;
    SkBitmap Bitmap;
    
    Display *fDisplay = nullptr;    
    XVisualInfo *fVi  = nullptr; // VisualInfo structure for GL
    Window fWin;
    GC fGc; // Graphic context

  private:

    void mapWindowAndWait();
    bool wndProc(XEvent *evt); // Returns false to exit the loop
    void resize(int width, int height);
    void paint();
  };

}

#endif // VARCO_BASEOSWINDOW_LINUX_HPP
