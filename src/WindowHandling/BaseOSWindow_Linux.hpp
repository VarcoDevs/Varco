#ifndef VARCO_BASEOSWINDOW_LINUX_HPP
#define VARCO_BASEOSWINDOW_LINUX_HPP

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/extensions/sync.h>

#include <Utils/Commons.hpp>
#include <Utils/VKeyCodes.hpp>
#include <SkCanvas.h>
#include <SkSurface.h>
#include <SkSurfaceProps.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

class GrContext;
struct GrGLInterface;
class GrRenderTarget;

namespace varco {

  class BaseOSWindow {
  public:

    BaseOSWindow(int argc, char **argv);
    virtual ~BaseOSWindow();

    int show();

    virtual void draw(SkCanvas& canvas) = 0;
    void repaint();
    virtual void onMouseMove(SkScalar x, SkScalar y) = 0;
    virtual void onLeftMouseDown(SkScalar x, SkScalar y) = 0;
    virtual void onLeftMouseMove(SkScalar x, SkScalar y) = 0;
    virtual void onMouseWheel(SkScalar x, SkScalar y, int direction) = 0;
    virtual void onFileDrop(SkScalar x, SkScalar y, std::vector<std::string> files) = 0;
    void startMouseCapture();
    void stopMouseCapture();
    virtual void onMouseLeave() = 0;
    virtual void onLeftMouseUp(SkScalar x, SkScalar y) = 0;
    virtual void onKeyDown(VirtualKeycode key) = 0;

  protected:
    int Argc;
    char **Argv;
    int Width, Height;
    SkBitmap Bitmap;    
    
    Display *fDisplay = nullptr;    
    XVisualInfo *fVi  = nullptr; // VisualInfo structure for GL
    int fMSAASampleCount;
    Window fWin;
    GC fGc; // Graphic context
    GLXContext fGLContext;
    GLXContext fSharedGLContext;

    std::unique_ptr<SkSurface> fSurface;
    std::unique_ptr<const SkSurfaceProps> fSurfaceProps;
    GrContext* fContext;
    GrRenderTarget* fRenderTarget;
    AttachmentInfo fAttachmentInfo;
    const int requestedMSAASampleCount = 0; // Modify this to increase MSAA
    const GrGLInterface* fInterface;
    GrRenderTarget* setupRenderTarget(int width, int heigth);

  private:

    void mapWindowAndWait();
    void invalidateWindow(bool flush = true);
    std::chrono::time_point<std::chrono::system_clock> lastResizeEventTime;
    std::unique_ptr<std::thread> redrawThread;
    bool fNeedDraw = true;
    bool wndProc(XEvent *evt); // Returns false to exit the loop
    void resize(int width, int height);
    void paint();

    std::mutex renderMutex;
    std::condition_variable renderCV;
    bool redrawNeeded = false;
    std::thread renderThread;
    void renderThreadFn();
    bool stopRendering = false;
    void setVsync(bool vsync);

    XSyncCounter netwm_sync_counter;
    XSyncValue netwm_sync_value;
    int flags = 0;

    bool m_XDNDInProgress = false; // Drag and drop currently in progress?
    SkPoint m_XDNDPos; // Position of current drag and drop
    std::vector<Atom> m_supportedXDNDMimes;
  };

}

#endif // VARCO_BASEOSWINDOW_LINUX_HPP
