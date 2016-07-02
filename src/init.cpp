#include <WindowHandling/MainWindow.hpp>
#ifdef _WIN32
  #include "windows.h"
#endif

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
  varco::MainWindow window(hInstance, hPrevInstance, lpCmdLine, nCmdShow);  
  return window.show();
}
#elif defined __linux__
int main(int argc, char **argv)
{
  varco::MainWindow window(argc, argv);
  return window.show();
}
#endif
