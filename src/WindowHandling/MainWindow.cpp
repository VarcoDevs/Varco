#include <WindowHandling/MainWindow.hpp>
#include "SkCanvas.h"
#include "SkGradientShader.h"
#include "SkGraphics.h"


namespace varco {

#ifdef WIN32
  MainWindow::MainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                         LPSTR lpCmdLine, int nCmdShow)
    : BaseOSWindow(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
#endif
  {}

  // Main window drawing entry point
  void MainWindow::draw(SkCanvas *canvas) {
    // Clear background color
    canvas->drawColor(SK_ColorWHITE);

    SkPaint paint;
    paint.setColor(SK_ColorRED);

    // Draw a rectangle with red paint
    SkRect rect = {
      10, 10,
      128, 128
    };
    canvas->drawRect(rect, paint);

    // Set up a linear gradient and draw a circle
    {
      SkPoint linearPoints[] = {
        { 0, 0 },
        { 300, 300 }
      };
      SkColor linearColors[] = { SK_ColorGREEN, SK_ColorBLACK };

      SkShader* shader = SkGradientShader::CreateLinear(
        linearPoints, linearColors, NULL, 2,
        SkShader::kMirror_TileMode);
      SkAutoUnref shader_deleter(shader);

      paint.setShader(shader);
      paint.setFlags(SkPaint::kAntiAlias_Flag);

      canvas->drawCircle(200, 200, 64, paint);

      // Detach shader
      paint.setShader(NULL);
    }

  }
} // namespace varco
