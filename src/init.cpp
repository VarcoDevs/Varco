#ifdef WIN32
  #include <WindowHandling/MainWindow.hpp>
  #include "windows.h"
#endif

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
  varco::MainWindow window(hInstance, hPrevInstance, lpCmdLine, nCmdShow);  
  return window.show();
}
#endif