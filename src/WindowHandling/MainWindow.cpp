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
    SkRect tabCtrlRect = SkRect::MakeLTRB(0, 0, this->Width, 30);
    // Draw the TabCtrl region if needed
    tabCtrl.setRect(tabCtrlRect);
    tabCtrl.paint(canvas);



//    // Set up a linear gradient and draw a circle
//    {
//      SkPoint linearPoints[] = {
//        { 0, 0 },
//        { 300, 300 }
//      };
//      SkColor linearColors[] = { SK_ColorGREEN, SK_ColorBLACK };

//      SkShader* shader = SkGradientShader::CreateLinear(
//        linearPoints, linearColors, NULL, 2,
//        SkShader::kMirror_TileMode);
//      SkAutoUnref shader_deleter(shader);

//      paint.setShader(shader);
//      paint.setFlags(SkPaint::kAntiAlias_Flag);

//      canvas->drawCircle(200, 200, 64, paint);

//      // Detach shader
//      paint.setShader(NULL);
//    }

  }
} // namespace varco
