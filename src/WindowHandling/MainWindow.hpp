#ifndef VARCO_MAINWINDOW_HPP
#define VARCO_MAINWINDOW_HPP

#ifdef _WIN32
  #include <WindowHandling/BaseOSWindow_Win.hpp>
#elif defined __linux__
  #include <WindowHandling/BaseOSWindow_Linux.hpp>
#endif
#include <UI/TabBar/TabBar.hpp>
#include <UI/CodeView/CodeView.hpp>
#include <Control/DocumentManager.hpp>
#include <SkCanvas.h>
#include <string>

namespace varco {

  class MainWindow : public BaseOSWindow, public UIElement<ui_container_tag> {
  public:

#ifdef _WIN32
    MainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
#elif defined __linux__
    MainWindow(int argc, char **argv);
#endif

    void draw(SkCanvas& canvas) override;
    void repaint() override;
    void onMouseMove(SkScalar x, SkScalar y) override;
    void onLeftMouseDown(SkScalar x, SkScalar y) override;
    void onLeftMouseMove(SkScalar x, SkScalar y) override;
    void onMouseWheel(SkScalar x, SkScalar y, int direction) override;
    void onFileDrop(SkScalar x, SkScalar y, std::vector<std::string> files) override;
    void onMouseLeave() override;
    void onLeftMouseUp(SkScalar x, SkScalar y) override;
    void onKeyDown(VirtualKeycode key) override;
    void startMouseCapture() override;
    void stopMouseCapture() override;

  private:
    // Warning: keep these in order
    // (per §12.6.2.5 these define the order for the ctor initialization list)
    TabBar m_tabCtrl;
    CodeView m_codeEditCtrl;
    DocumentManager m_documentManager;
  };

}

#endif // VARCO_MAINWINDOW_HPP
