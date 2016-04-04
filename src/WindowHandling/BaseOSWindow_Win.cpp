#include <WindowHandling/BaseOSWindow_Win.hpp>
#include <Utils/Utils.hpp>

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
  }

  int BaseOSWindow::show() {
    ShowWindow(hWnd, CmdShow);
    UpdateWindow(hWnd);

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }
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

        redraw();
      } break;

      case WM_PAINT: {

        /*MSG msg;
        while (PeekMessage(&msg, hWnd, 0, 0, PM_QS_PAINT)) {
        }*/

        if (!(Width > 0 && Height > 0))
          return 1; // Nonsense painting a 0 area
        else {
          if (Width != Bitmap.width() || Height != Bitmap.height())
            Bitmap.allocPixels(SkImageInfo::Make(Width, Height, kN32_SkColorType, kPremul_SkAlphaType));
        }

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Call the OS-independent draw function        
        SkCanvas canvas(this->Bitmap);
        this->draw(canvas);

        // Finally do the painting after the drawing is done
        this->paint(hdc, false);

        EndPaint(hWnd, &ps);

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

  // BitBlt the rendered window into the device context
  void BaseOSWindow::paint(HDC hdc, bool aero) {

    HDC hdcMem = CreateCompatibleDC(hdc);
    
    BITMAPINFO BMI;
    memset(&BMI, 0, sizeof(BMI));
    BMI.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    BMI.bmiHeader.biWidth = Bitmap.width();
    BMI.bmiHeader.biHeight = -Bitmap.height(); // Bitmap are stored top-down
    BMI.bmiHeader.biPlanes = 1;
    BMI.bmiHeader.biBitCount = 32;
    BMI.bmiHeader.biCompression = BI_RGB;
    BMI.bmiHeader.biSizeImage = 0;

    void *pixels = Bitmap.getPixels();
    HBITMAP bmp = CreateDIBSection(hdcMem, &BMI, DIB_RGB_COLORS, &pixels, NULL, NULL);

    SkASSERT(Bitmap.width() * Bitmap.bytesPerPixel() == Bitmap.rowBytes());
    Bitmap.lockPixels();
    SetDIBits(hdcMem, bmp, 0, Bitmap.height(),
      Bitmap.getPixels(),
      &BMI, DIB_RGB_COLORS);

    /* // Directly set pixels in the device context
    SetDIBitsToDevice(hdcMem,
      0, 0,
      Bitmap.width(), Bitmap.height(),
      0, 0,
      0, Bitmap.height(),
      Bitmap.getPixels(),
      &BMI,
      DIB_RGB_COLORS);*/
    Bitmap.unlockPixels();

    HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, bmp);

    BitBlt(hdc, 0, 0, Bitmap.width(), Bitmap.height(), hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, oldBmp);

    DeleteDC(hdcMem);
    DeleteObject(bmp);
  }

  void BaseOSWindow::resize(int width, int height) {
    this->Width = width;
    this->Height = height;    
  }  

} // namespace varco
