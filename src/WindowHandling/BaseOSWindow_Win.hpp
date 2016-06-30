#ifndef VARCO_BASEOSWINDOW_WIN_HPP
#define VARCO_BASEOSWINDOW_WIN_HPP

#include <Utils/WGL.hpp>
#include <Utils/Commons.hpp>
#include <Utils/VKeyCodes.hpp>
#include <SkCanvas.h>
#include <SkSurface.h>
#include "windows.h"
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>

class GrContext;
struct GrGLInterface;
class GrRenderTarget;

namespace varco {

  class BaseOSWindow {
  public:

    BaseOSWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, 
                 int nCmdShow);
    virtual ~BaseOSWindow();

    int show();
    
    virtual void draw(SkCanvas& canvas) = 0;
    void repaint();
    virtual void onMouseMove(SkScalar x, SkScalar y) = 0;
    virtual void onLeftMouseDown(SkScalar x, SkScalar y) = 0;
    virtual void onLeftMouseMove(SkScalar x, SkScalar y) = 0;
    virtual void onMouseWheel(SkScalar x, SkScalar y, int direction) = 0;
    virtual void onFileDrop(SkScalar x, SkScalar y, std::string file) = 0;
    void startMouseCapture();
    void stopMouseCapture();
    virtual void onMouseLeave() = 0;
    virtual void onLeftMouseUp(SkScalar x, SkScalar y) = 0;
    virtual void onKeyDown(VirtualKeycode key) = 0;

  protected:
    HINSTANCE Instance, PrevInstance;
    HWND hWnd;
    LPSTR CmdLine;
    int CmdShow;
    int Width, Height;
    SkBitmap Bitmap;

    void *fHGLRC;
    std::unique_ptr<SkSurface> fSurface;
    std::unique_ptr<const SkSurfaceProps> fSurfaceProps;
    GrContext* fContext;
    GrRenderTarget* fRenderTarget;
    AttachmentInfo fAttachmentInfo;
    const int requestedMSAASampleCount = 0; // Modify this to increase MSAA
    const GrGLInterface* fInterface;
    GrRenderTarget* setupRenderTarget(int width, int heigth);

  private:
    LRESULT wndProcInternal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> wndProc;

    void paint(HDC hdc, bool aero = false);
    void resize(int width, int height);

    bool mouseCaptureActive = false;

    std::unique_ptr<SkSurface> surface;

    std::mutex renderMutex;
    std::condition_variable renderCV;
    bool redrawNeeded = false;
    bool stopRendering = false;
    std::thread renderThread;
    void renderThreadFn();
  };

}

#endif // VARCO_BASEOSWINDOW_WIN_HPP
