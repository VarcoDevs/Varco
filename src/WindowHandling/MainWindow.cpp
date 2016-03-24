#include <WindowHandling/MainWindow.hpp>
#include "SkCanvas.h"
#include "SkGradientShader.h"
#include "SkGraphics.h"


namespace varco {

#ifdef _WIN32
  MainWindow::MainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                         LPSTR lpCmdLine, int nCmdShow)
    : BaseOSWindow(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
#elif defined __linux__
  MainWindow::MainWindow(int argc, char **argv)
    : BaseOSWindow(argc, argv)
#endif
  {}

  // Main window drawing entry point
  void MainWindow::draw(SkCanvas *canvas) {
    // Clear background color
    canvas->drawColor(SK_ColorWHITE);

    // Calculate TabCtrl region
    SkRect tabCtrlRect = SkRect::MakeLTRB(0, 0, (SkScalar)this->Width, 33);
    // Draw the TabCtrl region if needed
    tabCtrl.setRect(tabCtrlRect);
    tabCtrl.paint(canvas);
  }


  bool MainWindow::onMouseDown(int x, int y) {
    // Forward the event to the container control
    auto isPointInsideRect = [](SkScalar x, SkScalar y, SkRect rect) {
      if (rect.fLeft <= x && rect.fTop <= y && rect.fRight >= x && rect.fBottom >= y)
        return true;
      else
        return false;
    };
    SkScalar skX(static_cast<SkScalar>(x)), skY(static_cast<SkScalar>(y));
    if (isPointInsideRect(skX, skY, tabCtrl.getRect())) {
      return tabCtrl.onMouseClick(skX, skY);
    }
  }

} // namespace varco
