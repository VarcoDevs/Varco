#ifndef VARCO_BASEOSWINDOW_WIN_HPP
#define VARCO_BASEOSWINDOW_WIN_HPP

#include <SkCanvas.h>
#include <SkSurface.h>
#include "windows.h"
#include <functional>
#include <memory>

namespace varco {

  class BaseOSWindow {
  public:

    BaseOSWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, 
                 int nCmdShow);

    int show();

    virtual void draw(SkCanvas& canvas) = 0;
    virtual bool onMouseDown(SkScalar x, SkScalar y) = 0; // Returns true if a redraw is needed

  protected:
    HINSTANCE Instance, PrevInstance;
    HWND hWnd;
    LPSTR CmdLine;
    int CmdShow;
    int Width, Height;
    SkBitmap Bitmap;

  private:
    LRESULT wndProcInternal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> wndProc;

    void paint(HDC hdc, bool aero = false);
    void resize(int width, int height);

    std::unique_ptr<SkSurface> surface;
  };

}

#endif // VARCO_BASEOSWINDOW_WIN_HPP
