#ifndef VARCO_MAINWINDOW_HPP
#define VARCO_MAINWINDOW_HPP

#ifdef WIN32
#include "windows.h"
#endif

namespace varco {

  class MainWindow {
  public:

#ifdef WIN32
    static int create (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, 
                       int nCmdShow);
#endif
  };

}

#endif // VARCO_MAINWINDOW_HPP