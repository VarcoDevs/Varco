#ifndef VARCO_MAINWINDOW_HPP
#define VARCO_MAINWINDOW_HPP

#include <Utils/VKeyCodes.hpp>
#include <UI/TabBar/TabBar.hpp>
#include <UI/CodeView/CodeView.hpp>
#include <Control/DocumentManager.hpp>
#include <SkCanvas.h>
#include <string>

namespace varco {

  class MainWindow : public UIElement<ui_container_tag> {
  public:

    MainWindow(int argc, char **argv, int width, int height);

    void draw(SkCanvas& canvas);
    void repaint();
    void onMouseMove(SkScalar x, SkScalar y);
    void onLeftMouseDown(SkScalar x, SkScalar y);
    void onLeftMouseMove(SkScalar x, SkScalar y);
    void onMouseWheel(SkScalar x, SkScalar y, int direction);
    void onFileDrop(SkScalar x, SkScalar y, std::vector<std::string> files);
    void onMouseLeave();
    void onLeftMouseUp(SkScalar x, SkScalar y);
    void onKeyDown(VirtualKeycode key);
    void startMouseCapture();
    void stopMouseCapture();
    void resize(int width, int height);

  private:
    int m_width = 0, m_height = 0;
    // Warning: keep these in order
    // (per §12.6.2.5 these define the order for the ctor initialization list)
    TabBar m_tabCtrl;
    CodeView m_codeEditCtrl;
    DocumentManager m_documentManager;
  };

}

#endif // VARCO_MAINWINDOW_HPP
