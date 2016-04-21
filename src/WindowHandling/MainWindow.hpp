#ifndef VARCO_MAINWINDOW_HPP
#define VARCO_MAINWINDOW_HPP

#ifdef _WIN32
  #include <WindowHandling/BaseOSWindow_Win.hpp>
#elif defined __linux__
  #include <WindowHandling/BaseOSWindow_Linux.hpp>
#endif
#include <UI/TabCtrl/TabCtrl.hpp>
#include <UI/CodeEditCtrl/CodeEditCtrl.hpp>
#include <Document/Document.hpp>
#include <SkCanvas.h>
#include <memory>
#include <map>

namespace varco {

  class MainWindow : public BaseOSWindow {
  public:

#ifdef _WIN32
    MainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
#elif defined __linux__
    MainWindow(int argc, char **argv);
#endif

    void draw(SkCanvas& canvas) override;
    void onLeftMouseDown(SkScalar x, SkScalar y) override;
    void onLeftMouseMove(SkScalar x, SkScalar y) override;
    void onMouseLeave();
    void onLeftMouseUp(SkScalar x, SkScalar y) override;
    void onKeyDown(VirtualKeycode key) override;

  private:
    TabCtrl tabCtrl;
    CodeEditCtrl codeEditCtrl;

    // A map that stores the association between a tab and a document
    std::map<int, std::unique_ptr<Document>> m_tabDocumentMap;
  };

}

#endif // VARCO_MAINWINDOW_HPP
