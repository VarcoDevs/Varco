#ifdef WIN32
  #include "windows.h"
#endif
#include <WindowHandling/MainWindow.hpp>

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
  return varco::MainWindow::create(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
#endif