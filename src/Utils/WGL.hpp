#ifndef VARCO_WGL_HPP
#define VARCO_WGL_HPP

#define NOMINMAX // Damn windows.h macros
#include <windows.h>

namespace varco {

#define WGL_DRAW_TO_WINDOW                       0x2001
#define WGL_ACCELERATION                         0x2003
#define WGL_SUPPORT_OPENGL                       0x2010
#define WGL_DOUBLE_BUFFER                        0x2011
#define WGL_COLOR_BITS                           0x2014
#define WGL_ALPHA_BITS                           0x201B
#define WGL_STENCIL_BITS                         0x2023
#define WGL_FULL_ACCELERATION                    0x2027
#define WGL_SAMPLE_BUFFERS                       0x2041
#define WGL_SAMPLES                              0x2042
#define WGL_CONTEXT_MAJOR_VERSION                0x2091
#define WGL_CONTEXT_MINOR_VERSION                0x2092
#define WGL_CONTEXT_LAYER_PLANE                  0x2093
#define WGL_CONTEXT_FLAGS                        0x2094
#define WGL_CONTEXT_PROFILE_MASK                 0x9126
#define WGL_CONTEXT_DEBUG_BIT                    0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT       0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT             0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT    0x00000002
#define WGL_CONTEXT_ES2_PROFILE_BIT              0x00000004
#define ERROR_INVALID_VERSION                    0x2095

  class WGLExtensions {
  public:
    WGLExtensions();

    // Determine if an extension is available for a given dc
    bool hasExtension(HDC dc, const char* ext) const;

    const char* getExtensionsString(HDC hdc) const;

    BOOL choosePixelFormat(HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList,
      UINT nMaxFormats, int* piFormats, UINT* nNumFormats) const;

    // Selects a pixel format among the ones returned by wglChoosePixelFormat. Tries to keep
    // into consideration:
    //  - formats with samples >= desiredSampleCount
    //  - formats with fewest color samples when coverage sampling is available
    //  - get the first one which matches all of the above    
    int selectFormat(const int formats[], int formatCount, HDC dc, int desiredSampleCount) const;

    BOOL getPixelFormatAttribiv(HDC, int, int, UINT, const int*, int*) const;

    BOOL swapInterval(int interval) const;

  private:
    typedef const char* (WINAPI *GetExtensionsStringProc)(HDC);
    typedef BOOL(WINAPI *ChoosePixelFormatProc)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
    typedef BOOL (WINAPI *GetPixelFormatAttribivProc)(HDC, int, int, UINT, const int*, int*);
    typedef BOOL(WINAPI* SwapIntervalProc)(int);
    GetExtensionsStringProc fGetExtensionsString;
    ChoosePixelFormatProc fChoosePixelFormat;
    GetPixelFormatAttribivProc fGetPixelFormatAttribiv;
    SwapIntervalProc fSwapInterval;
  };

  // Create an OpenGL context for a dc using WGL
  HGLRC createWGLContext(HDC dc, int msaaSampleCount);

}

#endif // VARCO_WGL_HPP
