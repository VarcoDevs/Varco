#ifndef VARCO_MAINWINDOW_HPP
#define VARCO_MAINWINDOW_HPP

#ifdef WIN32
  #include <WindowHandling/BaseOSWindow_Win.hpp>
#endif

#include "SkCanvas.h"

namespace varco {

  class MainWindow : public BaseOSWindow {
  public:

#ifdef WIN32
    MainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, 
               int nCmdShow);
#endif

    void draw(SkCanvas *canvas) override;
  };

}

#endif // VARCO_MAINWINDOW_HPP