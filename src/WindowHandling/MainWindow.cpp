#include <WindowHandling/MainWindow.hpp>
#include <Debug/stubs.h>

namespace varco {

#ifdef WIN32

// Specify the windows subsystem
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
// Link Windows-specific stuff
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "UxTheme.lib")

#include <windows.h>
#include <dwmapi.h>
#include <vsstyle.h>
#include <windowsx.h>
#include <Uxtheme.h>

  // Borders for the hit-tests on the frameless window
  const int LEFTEXTENDWIDTH = 8;
  const int RIGHTEXTENDWIDTH = 8;
  const int TOPEXTENDWIDTH = 27;
  const int BOTTOMEXTENDWIDTH = 20;


  HBITMAP g_hbmBall = NULL;

  BOOL isCompositionEnabled = FALSE;


  // Hit test the frame for resizing and moving taken from
  // https://msdn.microsoft.com/en-us/library/bb688195(VS.85).aspx
  LRESULT HitTestNCA(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    // Get the point coordinates for the hit test.
    POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    // Get the window rectangle.
    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);

    // Get the frame rectangle, adjusted for the style without a caption.
    RECT rcFrame = { 0 };
    AdjustWindowRectEx(&rcFrame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, NULL);

    // Determine if the hit test is for resizing. Default middle (1,1).
    USHORT uRow = 1;
    USHORT uCol = 1;
    bool fOnResizeBorder = false;

    // Determine if the point is at the top or bottom of the window.
    if (ptMouse.y >= rcWindow.top && ptMouse.y < rcWindow.top + TOPEXTENDWIDTH) {
      fOnResizeBorder = (ptMouse.y < (rcWindow.top - rcFrame.top));
      uRow = 0;
    }
    else if (ptMouse.y < rcWindow.bottom && ptMouse.y >= rcWindow.bottom - BOTTOMEXTENDWIDTH) {
      uRow = 2;
    }

    // Determine if the point is at the left or right of the window.
    if (ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + LEFTEXTENDWIDTH) {
      uCol = 0; // left side
    }
    else if (ptMouse.x < rcWindow.right && ptMouse.x >= rcWindow.right - RIGHTEXTENDWIDTH) {
      uCol = 2; // right side
    }

    // Hit test (HTTOPLEFT, ... HTBOTTOMRIGHT)
    LRESULT hitTests[3][3] = {
      { HTTOPLEFT,    fOnResizeBorder ? HTTOP : HTCAPTION,    HTTOPRIGHT },
      { HTLEFT,       HTNOWHERE,     HTRIGHT },
      { HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT },
    };

    return hitTests[uRow][uCol];
  }

  LRESULT CALLBACK CompositionWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL& fCallDWP) {
    LRESULT lRet = 0;
    // Hit testing within non-client area
    fCallDWP = !DwmDefWindowProc(hWnd, message, wParam, lParam, &lRet);

    switch (message) {

      case WM_NCCALCSIZE: {

        if (wParam == TRUE) { // We need to indicate the valid client area
                              // Calculate new NCCALCSIZE_PARAMS based on custom NCA inset
                              // pncsp[0] = new window rectangle (in parent coordinates)
                              // pncsp[1] = old window rectangle(in parent coordinates)
                              // pncsp[2] = old client rectangle(in parent coordinates)
          NCCALCSIZE_PARAMS *pncsp = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
          // When returning, this is expected:
          // pncsp[0] = new client rectangle(in parent coordinates)

          // Make the entire non-client area as client drawable
          pncsp->rgrc[0].left = pncsp->rgrc[0].left + 0;
          pncsp->rgrc[0].top = pncsp->rgrc[0].top + 0;
          pncsp->rgrc[0].right = pncsp->rgrc[0].right - 0;
          pncsp->rgrc[0].bottom = pncsp->rgrc[0].bottom - 0;

          // No need to pass the message on to the DefWindowProc
          // This actually prevents the drawing of window icon and title

          lRet = WVR_VALIDRECTS; // [1] and [2] are valid. The system can use these
                                 // to combine and avoid redrawing some parts
          fCallDWP = false;
        }
      } break;
      case WM_NCHITTEST: {
        if (lRet != 0) // Only handle unhandled events
          break;

        lRet = HitTestNCA(hWnd, wParam, lParam);

        // Don't call DWP if we handled the signal with DwmDefWindowProc
        if (lRet != HTNOWHERE)
          fCallDWP = false;

      } break;

    }

    return lRet;
  }

  LRESULT CALLBACK AfterCompositionDefaultWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

      case WM_CLOSE: {
        DestroyWindow(hWnd);
      } break;

      case WM_DESTROY: {
        PostQuitMessage(0);
      } break;

      case WM_ACTIVATE: {
        // Extend the frame into the client area
        MARGINS margins{ -1, -1, -1, -1 };

        HRESULT hr = DwmExtendFrameIntoClientArea(hWnd, &margins);
        if (!SUCCEEDED(hr)) {
          MessageBox(NULL, "DWM frame extension failed", "Error", MB_ICONEXCLAMATION);
          return 0;
        }

      } break;

      case WM_CREATE: {
        RECT rcClient;
        GetWindowRect(hWnd, &rcClient);

        // Inform the application of the frame change - triggers a WM_NCCALCSIZE
        // and removes the window title and icon
        SetWindowPos(hWnd, NULL, rcClient.left, rcClient.top,
          rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
          SWP_FRAMECHANGED);

        SetCursor(LoadCursor(NULL, IDC_ARROW));

        SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_LAYERED);
        // Choose a colour that you will not use in the program, eg RGB(200,201,202)
        // This color will actually be rendered as transparent
        SetLayeredWindowAttributes(hWnd, RGB(181, 230, 29), 0, LWA_COLORKEY);

      } break;

      // TODO: handle resizing (must paint everything in fullscreen as well)

      case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetWindowRect(hWnd, &rect);

        HDC hdcMem = CreateCompatibleDC(hdc);
        BITMAP bm;

        // Draw a bitmap with a transparent background on to the client area
        GetObject(g_hbmBall, sizeof(bm), &bm);

        HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, g_hbmBall);

        BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, oldBmp);

        EndPaint(hWnd, &ps);
      } break;

    }
    return DefWindowProc(hWnd, message, wParam, lParam);
  }

  // Warning: little-endian only
  COLOR16 R16(COLORREF c) {
    return (GetRValue(c) << 8);
  };
  COLORREF G16(COLORREF c) {
    return (COLOR16)(GetGValue(c) << 8);
  };
  COLORREF B16(COLORREF c) {
    return (COLOR16)(GetBValue(c) << 8);
  };
  COLORREF R16(COLORREF c0, COLORREF c1) {
    return ((GetRValue(c0) + GetRValue(c1)) / 2) << 8;
  };
  COLORREF G16(COLORREF c0, COLORREF c1) {
    return ((GetGValue(c0) + GetGValue(c1)) / 2) << 8;
  };
  COLORREF B16(COLORREF c0, COLORREF c1) {
    return ((GetBValue(c0) + GetBValue(c1)) / 2) << 8;
  };

  BOOL GradientHorizontalRectangle(HDC hDC, int x0, int y0, int x1, int y1,
    COLORREF c0, COLORREF c1) {
    const int empirical_gradient_offset = 50;
    TRIVERTEX vert[2] = {
      { x0, y0, (COLOR16)R16(c0),(COLOR16)G16(c0), (COLOR16)B16(c0), 0 },
      { x1 - empirical_gradient_offset /* Empirical offset */, y1, (COLOR16)R16(c1),(COLOR16)G16(c1), (COLOR16)B16(c1), 0 }
    };
    ULONG Index[] = { 0, 1 }; // First is upperleft, second is lowerright
    BOOL res = GradientFill(hDC, vert, 2, Index, 1, GRADIENT_FILL_RECT_H);
    RECT rect;
    rect.left = vert[1].x;
    rect.top = vert[0].y;
    rect.right = x1;
    rect.bottom = y1;
    HBRUSH br = CreateSolidBrush(c1);
    FillRect(hDC, &rect, br);
    return res;
  }

  LRESULT CALLBACK StandardWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    RECT wr;
    RECT dirty;

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

      case WM_ACTIVATE:
      case WM_SIZE:
      case WM_ACTIVATEAPP:
      case WM_NCACTIVATE: {
        // Paint the non-client area now, otherwise Windows will paint its own
        RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW);
        return 0;
      } break;

      case WM_NCPAINT: {

        // Paint window frame first
        auto res = DefWindowProc(hWnd, message, wParam, lParam);

        // Then paint a bitmap with a transparency color over
        GetWindowRect(hWnd, &wr);
        LONG width = wr.right - wr.left;
        LONG height = wr.bottom - wr.top;
        dirty = wr;
        dirty.left = dirty.top = 5;
        dirty.right = width - 5;
        dirty.bottom = height - 5;;

        HDC hdc = GetWindowDC(hWnd);


        //----------------------------------------
        // No visual style is applied, draw a gradient background on the window
        // the same way as the title bar (if no theme is applied)
        POINT pt;
        pt.x = 0;
        pt.y = 0;

        ClientToScreen(hWnd, &pt);
        
        // pt contains screen coordinates of left-top hWnd client point

        RECT crect, rect;
        GetClientRect(hWnd, &crect);
        GetWindowRect(hWnd, &rect);

        crect.left = pt.x - rect.left;
        crect.right += crect.left;
        crect.top = pt.y - rect.top - 2 /* Eat the border out */;
        crect.bottom += crect.top + 2;

        DWORD start = GetSysColor(COLOR_ACTIVECAPTION), end = GetSysColor(COLOR_GRADIENTACTIVECAPTION);

        if (IsAppThemed() == FALSE) {
          // no aero and no composition
          GradientHorizontalRectangle(hdc, crect.left, crect.top, crect.right, crect.bottom, start, end);
        }
        else {
          // no aero but composition (themes) is on
          HBRUSH br = CreateSolidBrush(end);
          FillRect(hdc, &crect, br);
        }

        BITMAP bm;
        HDC hdcMem = CreateCompatibleDC(hdc);
        GetObject(g_hbmBall, sizeof(bm), &bm);
        HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, g_hbmBall);

        TransparentBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, RGB(181, 230, 29));

        SelectObject(hdcMem, oldBmp);
        ReleaseDC(hWnd, hdc);
        //----------------------------------------

        return res; // This was handled

      } break;

      case WM_PAINT: {
        // All the paint work is done in the ncpaint message
      } break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
  }

  LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT lRet = 0;
    HRESULT hr = S_OK;

    // Check if composition is enabled (always on in Win8+)
    hr = DwmIsCompositionEnabled(&isCompositionEnabled);
    if (SUCCEEDED(hr) == false)
      return -1;

    BOOL fCallDWP = false;
    if (isCompositionEnabled) {
      // Call the composition WndProc first..
      lRet = CompositionWndProc(hWnd, message, wParam, lParam, fCallDWP);
      // .. then the worker for the rest of the application
      if (fCallDWP)
        lRet = AfterCompositionDefaultWinProc(hWnd, message, wParam, lParam);
    }
    else
      lRet = StandardWndProc(hWnd, message, wParam, lParam);

    return lRet;
  }

  static const char class_name[] = "Varco";

  int MainWindow::create(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                         LPSTR lpCmdLine, int nCmdShow)
  {
    WNDCLASSEX wc{ 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = class_name;
    wc.hbrBackground = NULL; // No background

    if (!RegisterClassEx(&wc)) {
      MessageBox(NULL, "Window registration failed", "Error", MB_ICONEXCLAMATION);
      return 0;
    }

    HWND hWnd = CreateWindowEx(0, class_name, class_name, WS_OVERLAPPEDWINDOW, 
                               CW_USEDEFAULT, CW_USEDEFAULT, 545, 355, 0, 0, hInstance, 0);
    if (hWnd == nullptr) {
      MessageBox(NULL, "Window creation failed", "Error", MB_ICONEXCLAMATION);
      return 0;
    }

    g_hbmBall = (HBITMAP)LoadImage(GetModuleHandle(NULL), TESTBMP, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (g_hbmBall == NULL)
      MessageBox(hWnd, "Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
  }
#endif
}