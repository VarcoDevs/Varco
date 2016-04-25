#include <Utils/WGL.hpp>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <vector>

namespace varco {

#define GET_PROC(NAME, SUFFIX) f##NAME = \
                (##NAME##Proc) wglGetProcAddress("wgl" #NAME #SUFFIX)

  HWND create_dummy_window() {
    HMODULE module = GetModuleHandle(nullptr);
    HWND dummy;
    RECT windowRect;
    windowRect.left = 0;
    windowRect.right = 8;
    windowRect.top = 0;
    windowRect.bottom = 8;

    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)DefWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = module;
    wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = "DummyClass";

    if (!RegisterClass(&wc)) {
      return 0;
    }

    DWORD style, exStyle;
    exStyle = WS_EX_CLIENTEDGE;
    style = WS_SYSMENU;

    AdjustWindowRectEx(&windowRect, style, false, exStyle);
    if (!(dummy = CreateWindowEx(exStyle,
      "DummyClass",
      "DummyWindow",
      WS_CLIPSIBLINGS | WS_CLIPCHILDREN | style,
      0, 0,
      windowRect.right - windowRect.left,
      windowRect.bottom - windowRect.top,
      nullptr, nullptr,
      module,
      nullptr))) {
      UnregisterClass("DummyClass", module);
      return nullptr;
    }
    ShowWindow(dummy, SW_HIDE);

    return dummy;
  }

  void destroy_dummy_window(HWND dummy) {
    DestroyWindow(dummy);
    HMODULE module = GetModuleHandle(nullptr);
    UnregisterClass("DummyClass", module);
  }

  WGLExtensions::WGLExtensions() 
    : fGetExtensionsString(nullptr) 
  {
    HDC prevDC = wglGetCurrentDC();
    HGLRC prevGLRC = wglGetCurrentContext();

    PIXELFORMATDESCRIPTOR dummyPFD;

    ZeroMemory(&dummyPFD, sizeof(dummyPFD));
    dummyPFD.nSize = sizeof(dummyPFD);
    dummyPFD.nVersion = 1;
    dummyPFD.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    dummyPFD.iPixelType = PFD_TYPE_RGBA;
    dummyPFD.cColorBits = 32;
    dummyPFD.cDepthBits = 0;
    dummyPFD.cStencilBits = 8;
    dummyPFD.iLayerType = PFD_MAIN_PLANE;
    HWND dummyWND = create_dummy_window();
    if (dummyWND) {
      HDC dummyDC = GetDC(dummyWND);
      int dummyFormat = ChoosePixelFormat(dummyDC, &dummyPFD);
      SetPixelFormat(dummyDC, dummyFormat, &dummyPFD);
      HGLRC dummyGLRC = wglCreateContext(dummyDC);
      if (dummyGLRC == nullptr)
        throw std::runtime_error("Could not create dummy wnd to test wgl extensions");
      wglMakeCurrent(dummyDC, dummyGLRC);

      GET_PROC(GetExtensionsString, ARB);
      GET_PROC(ChoosePixelFormat, ARB);
      GET_PROC(GetPixelFormatAttribiv, ARB);
      GET_PROC(SwapInterval, EXT);

      wglMakeCurrent(dummyDC, nullptr);
      wglDeleteContext(dummyGLRC);
      destroy_dummy_window(dummyWND);
    }

    wglMakeCurrent(prevDC, prevGLRC);
  }


  const char* WGLExtensions::getExtensionsString(HDC hdc) const {
    return fGetExtensionsString(hdc);
  }

  bool WGLExtensions::hasExtension(HDC dc, const char* ext) const {
    if (nullptr == this->fGetExtensionsString) {
      return false;
    }
    if (!strcmp("WGL_ARB_extensions_string", ext)) {
      return true;
    }
    const char* extensionString = getExtensionsString(dc);
    size_t extLength = strlen(ext);

    while (true) {
      size_t n = strcspn(extensionString, " ");
      if (n == extLength && 0 == strncmp(ext, extensionString, n)) {
        return true;
      }
      if (0 == extensionString[n]) {
        return false;
      }
      extensionString += n + 1;
    }

    return false;
  }

  BOOL WGLExtensions::choosePixelFormat(HDC hdc,
    const int* piAttribIList,
    const FLOAT* pfAttribFList,
    UINT nMaxFormats,
    int* piFormats,
    UINT* nNumFormats) const {
    return fChoosePixelFormat(hdc, piAttribIList, pfAttribFList,
      nMaxFormats, piFormats, nNumFormats);
  }

  static void get_pixel_formats_to_try(HDC dc, const WGLExtensions& extensions,
    bool doubleBuffered, int msaaSampleCount,
    int formatsToTry[2]) {
    int iAttrs[] = {
      WGL_DRAW_TO_WINDOW, TRUE,
      WGL_DOUBLE_BUFFER, (doubleBuffered ? TRUE : FALSE),
      WGL_ACCELERATION, WGL_FULL_ACCELERATION,
      WGL_SUPPORT_OPENGL, TRUE,
      WGL_COLOR_BITS, 24,
      WGL_ALPHA_BITS, 8,
      WGL_STENCIL_BITS, 8,
      0, 0
    };

    float fAttrs[] = { 0, 0 };

    // Get a MSAA format if requested and possible.
    if (msaaSampleCount > 0 &&
      extensions.hasExtension(dc, "WGL_ARB_multisample")) {
      static const int kIAttrsCount = sizeof(iAttrs) / sizeof(iAttrs[0]);
      int msaaIAttrs[kIAttrsCount + 4];
      memcpy(msaaIAttrs, iAttrs, sizeof(int) * kIAttrsCount);
      assert(0 == msaaIAttrs[kIAttrsCount - 2] &&
        0 == msaaIAttrs[kIAttrsCount - 1]);
      msaaIAttrs[kIAttrsCount - 2] = WGL_SAMPLE_BUFFERS;
      msaaIAttrs[kIAttrsCount - 1] = TRUE;
      msaaIAttrs[kIAttrsCount + 0] = WGL_SAMPLES;
      msaaIAttrs[kIAttrsCount + 1] = msaaSampleCount;
      msaaIAttrs[kIAttrsCount + 2] = 0;
      msaaIAttrs[kIAttrsCount + 3] = 0;
      unsigned int num;
      int formats[64];
      extensions.choosePixelFormat(dc, msaaIAttrs, fAttrs, 64, formats, &num);
      num = std::min(num, 64U);
      formatsToTry[0] = extensions.selectFormat(formats, num, dc, msaaSampleCount);
    }

    // Get a non-MSAA format
    int* format = -1 == formatsToTry[0] ? &formatsToTry[0] : &formatsToTry[1];
    unsigned int num;
    extensions.choosePixelFormat(dc, iAttrs, fAttrs, 1, format, &num);
  }

  BOOL WGLExtensions::getPixelFormatAttribiv(HDC hdc, int iPixelFormat, int iLayerPlane, 
    UINT nAttributes, const int *piAttributes, int *piValues) const 
  {
    return fGetPixelFormatAttribiv(hdc, iPixelFormat, iLayerPlane,
      nAttributes, piAttributes, piValues);
  }

  namespace {

    struct PixelFormat {
      int fFormat;
      int fSampleCnt;
      int fChoosePixelFormatRank;
    };

    bool pf_less(const PixelFormat& a, const PixelFormat& b) {
      if (a.fSampleCnt < b.fSampleCnt) {
        return true;
      }
      else if (b.fSampleCnt < a.fSampleCnt) {
        return false;
      }
      else if (a.fChoosePixelFormatRank < b.fChoosePixelFormatRank) {
        return true;
      }
      return false;
    }

  }

  int WGLExtensions::selectFormat(const int formats[], int formatCount, HDC dc,
                                  int desiredSampleCount) const 
  {
    if (formatCount <= 0)
      return -1;

    PixelFormat desiredFormat = { 0, desiredSampleCount, 0 };
    std::vector<PixelFormat> rankedFormats(formatCount);
    for (int i = 0; i < formatCount; ++i) {
      static const int kQueryAttr = WGL_SAMPLES;
      int numSamples;
      this->getPixelFormatAttribiv(dc, formats[i], 0, 1, &kQueryAttr, &numSamples);
      rankedFormats[i].fFormat = formats[i];
      rankedFormats[i].fSampleCnt = numSamples;
      rankedFormats[i].fChoosePixelFormatRank = i;
    }
    std::sort(rankedFormats.begin(), rankedFormats.begin() + rankedFormats.size() - 1, pf_less);
    auto it = std::find_if(rankedFormats.begin(), rankedFormats.end(), [&](const auto& el) {
      return (desiredFormat.fFormat == el.fFormat &&
        desiredFormat.fSampleCnt == el.fSampleCnt &&
        desiredFormat.fChoosePixelFormatRank == el.fChoosePixelFormatRank);
    });
    if (it == rankedFormats.end())
      return -1;
    else
      return it->fFormat;
  }

  BOOL WGLExtensions::swapInterval(int interval) const {
    return fSwapInterval(interval);
  }

  static HGLRC create_gl_context(HDC dc, WGLExtensions extensions) {
    HDC prevDC = wglGetCurrentDC();
    HGLRC prevGLRC = wglGetCurrentContext();

    HGLRC glrc = wglCreateContext(dc); // Standard compatibility (or core) profile
    assert(glrc);

    wglMakeCurrent(prevDC, prevGLRC);

    // This might help make the context non-vsynced
    if (extensions.hasExtension(dc, "WGL_EXT_swap_control"))
      extensions.swapInterval(1);

    return glrc;
  }

  HGLRC createWGLContext(HDC dc, int msaaSampleCount) {
    WGLExtensions extensions;
    if (!extensions.hasExtension(dc, "WGL_ARB_pixel_format"))
      return nullptr;

    BOOL set = FALSE;

    int pixelFormatsToTry[] = { -1, -1 };
    get_pixel_formats_to_try(dc, extensions, true, msaaSampleCount, pixelFormatsToTry);
    for (int f = 0;
        !set && 
        pixelFormatsToTry[f] != -1 && 
        f < (sizeof(pixelFormatsToTry) / sizeof(pixelFormatsToTry[0]));
        ++f) 
    {
      PIXELFORMATDESCRIPTOR pfd;
      DescribePixelFormat(dc, pixelFormatsToTry[f], sizeof(pfd), &pfd);
      set = SetPixelFormat(dc, pixelFormatsToTry[f], &pfd);
    }

    if (!set) {
      return nullptr;
    }

    return create_gl_context(dc, extensions);
  }

}