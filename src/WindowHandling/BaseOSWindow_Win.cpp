#include <WindowHandling/BaseOSWindow_Win.hpp>
#include <gl/GrGLInterface.h>
#include <gl/GrGLUtil.h>
#include <GrContext.h>
#include <Utils/Utils.hpp>
#include <GL/gl.h>
#include <stdexcept>

// Specify the windows subsystem
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

namespace {

  // Helper templates to enable std::function<> to static routine bindings
  template <typename Unique, typename Ret, typename... Args>
  struct std_fn_helper {
  public:
    typedef std::function<Ret(Args...)> function_type;

    template<typename T>
    static void bind(T&& std_fun) {
      instance().std_fun_internal = std::forward<T>(std_fun);
    }

    // The entire point of this helper is here: to get a pointer to this
    // static function which will call the internal std::function<>
    static Ret invoke(Args... args) {
      return instance().std_fun_internal(args...);
    }
    using pointer_type = decltype(&std_fn_helper::invoke);

    static pointer_type ptr() {
      return &invoke;
    }

  private:
    std_fn_helper() = default;

    // Singleton for this instance (assumes this template instance is unique)
    static std_fn_helper& instance() {
      static std_fn_helper inst_;
      return inst_;
    }

    function_type std_fun_internal;
  };

  template <typename Unique, typename Ret, typename... Args>
  auto std_fn_to_static_fn_helper(const std::function<Ret(Args...)>& std_fun) {
    std_fn_helper<Unique, Ret, Args...>::bind(std_fun);
    return std_fn_helper<Unique, Ret, Args...>::ptr();
  }

  // Guaranteed to be unique by [expr.prim.lambda]/3
#define std_fn_to_static_fn(fn, fptr) do { \
      auto ll = [](){}; \
      fptr = std_fn_to_static_fn_helper<decltype(ll)>(fn); \
    } while(0)
}

namespace varco {

  static const char ClassName[] = "Varco";

  BaseOSWindow::BaseOSWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
    : Instance(hInstance), PrevInstance(hPrevInstance),
    CmdLine(lpCmdLine), CmdShow(nCmdShow), Width(545), Height(355)
  {
    using namespace std::placeholders;

    WNDCLASSEX WC{ 0 };
    WC.cbSize = sizeof(WNDCLASSEX);

    wndProc = std::bind(&BaseOSWindow::wndProcInternal, this, _1, _2, _3, _4);
    // Convert a std::function pointing to the wndProc to a static function
    // pointer and assign it to lpfnWndProc
    std_fn_to_static_fn(wndProc, WC.lpfnWndProc);
 
    WC.lpszClassName = ClassName;
    WC.hbrBackground = NULL; // No background
    // Todo set icon properly
    WC.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    if (!RegisterClassEx(&WC)) {
      MessageBox(NULL, "Window registration failed", "Error", MB_ICONEXCLAMATION);
      return;
    }

    hWnd = CreateWindowEx(0, ClassName, ClassName, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, 0, 0, hInstance, 0);

    if (hWnd == nullptr) {
      MessageBox(NULL, "Window creation failed", "Error", MB_ICONEXCLAMATION);
      return;
    }

    this->resize(this->Width, this->Height);

    // Prepare GL context
    HDC dc = GetDC((HWND)hWnd);
    fHGLRC = createWGLContext(dc, requestedMSAASampleCount);
    if (fHGLRC == nullptr)
      throw std::runtime_error("Could not create GL context");    

    if (!wglMakeCurrent(dc, (HGLRC)fHGLRC))
      throw std::runtime_error("Could not set the GL context");

    // use DescribePixelFormat to get the stencil bit depth
    int pixelFormat = GetPixelFormat(dc);
    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(dc, pixelFormat, sizeof(pfd), &pfd);
    fAttachmentInfo.fStencilBits = pfd.cStencilBits;

    // Get sample count if the MSAA WGL extension is present
    WGLExtensions extensions;
    if (extensions.hasExtension(dc, "WGL_ARB_multisample")) {
      static const int kSampleCountAttr = WGL_SAMPLES;
      extensions.getPixelFormatAttribiv(dc, pixelFormat, 0, 1, &kSampleCountAttr,
        &fAttachmentInfo.fSampleCount);
    } else
      fAttachmentInfo.fSampleCount = 0;

    glViewport(0, 0,
      SkScalarRoundToInt(this->Width),
      SkScalarRoundToInt(this->Height));

    glClearStencil(0);
    glClearColor(0, 0, 0, 0);
    glStencilMask(0xffffffff);
    glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    fSurfaceProps = std::make_unique<const SkSurfaceProps>(SkSurfaceProps::kLegacyFontHost_InitType);

    ReleaseDC(hWnd, dc);
    wglMakeCurrent(NULL, NULL);
  }

  BaseOSWindow::~BaseOSWindow() {
    wglMakeCurrent(GetDC((HWND)hWnd), 0);
    wglDeleteContext((HGLRC)fHGLRC);
  }

  GrRenderTarget* BaseOSWindow::setupRenderTarget(int width, int heigth) {
    GrBackendRenderTargetDesc desc;
    desc.fWidth = SkScalarRoundToInt(width);
    desc.fHeight = SkScalarRoundToInt(heigth);
    desc.fConfig = fContext->caps()->srgbSupport() &&
      (Bitmap.info().profileType() == kSRGB_SkColorProfileType ||
        Bitmap.info().colorType() == kRGBA_F16_SkColorType)
      ? kSkiaGamma8888_GrPixelConfig
      : kSkia8888_GrPixelConfig;
    desc.fOrigin = kBottomLeft_GrSurfaceOrigin;
    desc.fSampleCnt = fAttachmentInfo.fSampleCount;
    desc.fStencilBits = fAttachmentInfo.fStencilBits;
    GrGLint buffer;
    GR_GL_GetIntegerv(fInterface, GR_GL_FRAMEBUFFER_BINDING, &buffer);
    desc.fRenderTargetHandle = buffer;
    return fContext->textureProvider()->wrapBackendRenderTarget(desc);
  }

  int BaseOSWindow::show() {
    ShowWindow(hWnd, CmdShow);
    UpdateWindow(hWnd);

    std::function<void(void)> renderProc = std::bind(&BaseOSWindow::renderThreadFn, this);
    renderThread = std::thread(renderProc);

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }
    renderThread.detach();
    return static_cast<int>(Msg.wParam);
  }

  namespace {
    VirtualKeycode remapKeyToVarcoKey(WPARAM param) { // Windows-specific keycodes remapping
      using enumType = std::underlying_type<VirtualKeycode>::type;

      if (param >= 'A' && param <= 'Z')
        return static_cast<VirtualKeycode>(static_cast<enumType>(VirtualKeycode::VK_A) + (param - 'A'));
      else if (param >= '0' && param <= '9')
        return static_cast<VirtualKeycode>(static_cast<enumType>(VirtualKeycode::VK_0) + (param - '0'));
      else
        return VirtualKeycode::VK_UNRECOGNIZED;
    }
  }

  LRESULT BaseOSWindow::wndProcInternal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

      case WM_CLOSE: {
        DestroyWindow(hWnd);
      } break;

      case WM_DESTROY: {
        PostQuitMessage(0);
      } break;

      case WM_ERASEBKGND: {
        return 1; // Do not draw the background
      } break;

      case WM_LBUTTONDOWN: {
        auto x = LOWORD(lParam);
        auto y = HIWORD(lParam);
        this->onLeftMouseDown(x, y);
      } break;

      case WM_KEYDOWN: {
        this->onKeyDown(remapKeyToVarcoKey(wParam));
      } break;

      case WM_MOUSEMOVE: {
        if (wParam & MK_LBUTTON) { // Left mouse is down
          auto x = LOWORD(lParam);
          auto y = HIWORD(lParam);
          this->onLeftMouseMove(x, y);
        }
      } break;

      case WM_MOUSELEAVE: {
        this->onMouseLeave();
      } break;

      case WM_LBUTTONUP: {
        auto x = LOWORD(lParam);
        auto y = HIWORD(lParam);
        this->onLeftMouseUp(x, y);
      } break;

      case WM_SIZE: {
        auto width = LOWORD(lParam);
        auto height = HIWORD(lParam);
        this->resize(width, height);

        std::unique_lock<std::mutex> lk(renderMutex);
        redrawNeeded = true;
        renderCV.notify_one();
      } break;

      case WM_PAINT: {

        std::unique_lock<std::mutex> lk(renderMutex);
        redrawNeeded = true;
        renderCV.notify_one();

        return 0; // Completely handled

      } break;

    }
    return DefWindowProc(hWnd, message, wParam, lParam);
  }

  void BaseOSWindow::startMouseCapture() { // Track the mouse to be notified when it leaves the client area
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = this->hWnd;
    TrackMouseEvent(&tme);
  }

  void BaseOSWindow::redraw() {
    InvalidateRect(this->hWnd, nullptr, FALSE); // Send a WM_PAINT
  }

  void BaseOSWindow::renderThreadFn() {
    {
      std::unique_lock<std::mutex> lk(renderMutex);

      HDC dc = GetDC((HWND)hWnd);
      auto res = wglMakeCurrent(dc, (HGLRC)fHGLRC);
      if (res == false)
        throw std::runtime_error("Could not attach OpenGL context");

      fInterface = GrGLCreateNativeInterface();
      if (fInterface == nullptr)
        throw std::runtime_error("Could not create an OpenGL backend native interface");

      fContext = GrContext::Create(kOpenGL_GrBackend, (GrBackendContext)fInterface);
      if (fContext == nullptr)
        throw std::runtime_error("Could not create an OpenGL backend");

      fRenderTarget = setupRenderTarget(Width, Height); // render target has to be reset
      fSurface.reset(SkSurface::MakeRenderTargetDirect(fRenderTarget, fSurfaceProps.get()).release());

      ReleaseDC(hWnd, dc);

      //setVsync(false);
    }

    int threadWidth = Width, threadHeight = Height;

    while (true) {

      std::unique_lock<std::mutex> lk(renderMutex);
      while (!redrawNeeded) {
        renderCV.wait(lk);
        if (!redrawNeeded) {} // Spurious wakeup
      }

      if (!(Width > 0 && Height > 0))
        continue; // Nonsense painting a 0 area

      if (stopRendering == true)
        return;

      bool resizing = false;
      if (Width != threadWidth || Height != threadHeight) {
        // Area has been changed, we're resizing
        resizing = true;
      }

      HDC dc = GetDC((HWND)hWnd);
      auto res = wglMakeCurrent(dc, (HGLRC)fHGLRC);
      if (res == false)
        throw std::runtime_error("Could not attach OpenGL context");

      if (Width != threadWidth || Height != threadHeight) {
        fRenderTarget = setupRenderTarget(Width, Height); // render target has to be reset
        fSurface.reset(SkSurface::MakeRenderTargetDirect(fRenderTarget, fSurfaceProps.get()).release());
        threadWidth = Width;
        threadHeight = Height;
      }

      // Call the OS-independent draw function
      auto surfacePtr = fSurface->getCanvas();
      this->draw(*surfacePtr);

      if (stopRendering == true)
        return;

      //if (resizing) {
      //   std::this_thread::sleep_for(std::chrono::milliseconds(10));
      // } else {



      fContext->flush();
      wglSwapLayerBuffers(dc, WGL_SWAP_MAIN_PLANE);
      //glXSwapBuffers(fDisplay, fWin);

      if (Width == threadWidth && Height == threadHeight) {// Check for size to be updated
        redrawNeeded = false;
      }

      wglMakeCurrent(NULL, NULL);

      //std::this_thread::sleep_for(std::chrono::milliseconds(5));

      ReleaseDC(hWnd, dc);
    }
  }

  // BitBlt the rendered window into the device context
  void BaseOSWindow::paint(HDC hdc, bool aero) {

    return;

    //HDC hdcMem = CreateCompatibleDC(hdc);
    //
    //BITMAPINFO BMI;
    //memset(&BMI, 0, sizeof(BMI));
    //BMI.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    //BMI.bmiHeader.biWidth = Bitmap.width();
    //BMI.bmiHeader.biHeight = -Bitmap.height(); // Bitmap are stored top-down
    //BMI.bmiHeader.biPlanes = 1;
    //BMI.bmiHeader.biBitCount = 32;
    //BMI.bmiHeader.biCompression = BI_RGB;
    //BMI.bmiHeader.biSizeImage = 0;

    //void *pixels = Bitmap.getPixels();
    //HBITMAP bmp = CreateDIBSection(hdcMem, &BMI, DIB_RGB_COLORS, &pixels, NULL, NULL);

    //SkASSERT(Bitmap.width() * Bitmap.bytesPerPixel() == Bitmap.rowBytes());
    //Bitmap.lockPixels();
    //SetDIBits(hdcMem, bmp, 0, Bitmap.height(),
    //  Bitmap.getPixels(),
    //  &BMI, DIB_RGB_COLORS);

    ///* // Directly set pixels in the device context
    //SetDIBitsToDevice(hdcMem,
    //  0, 0,
    //  Bitmap.width(), Bitmap.height(),
    //  0, 0,
    //  0, Bitmap.height(),
    //  Bitmap.getPixels(),
    //  &BMI,
    //  DIB_RGB_COLORS);*/
    //Bitmap.unlockPixels();

    //HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, bmp);

    //BitBlt(hdc, 0, 0, Bitmap.width(), Bitmap.height(), hdcMem, 0, 0, SRCCOPY);

    //SelectObject(hdcMem, oldBmp);

    //DeleteDC(hdcMem);
    //DeleteObject(bmp);
  }

  void BaseOSWindow::resize(int width, int height) {
    this->Width = width;
    this->Height = height;    
  }  

} // namespace varco
