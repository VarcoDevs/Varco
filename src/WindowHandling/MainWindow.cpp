#include <WindowHandling/MainWindow.hpp>
#include <SkCanvas.h>
#include <Utils/Utils.hpp>


namespace varco {

#ifdef _WIN32
  MainWindow::MainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) : 
    BaseOSWindow(hInstance, hPrevInstance, lpCmdLine, nCmdShow),
    tabCtrl(*this)
#elif defined __linux__
  MainWindow::MainWindow(int argc, char **argv)
    : BaseOSWindow(argc, argv)
#endif
  {}

  // Main window drawing entry point
  void MainWindow::draw(SkCanvas& canvas) {
    // Clear background color
    canvas.drawColor(SkColorSetARGB(255, 39, 40, 34));

    // Calculate TabCtrl region
    SkRect tabCtrlRect = SkRect::MakeLTRB(0, 0, (SkScalar)this->Width, 33.0f);
    // Draw the TabCtrl region if needed
    tabCtrl.resize(tabCtrlRect);
    tabCtrl.paint();
    canvas.drawBitmap(tabCtrl.getBitmap(), tabCtrlRect.fLeft, tabCtrlRect.fTop);

    // TODO
    SkRect editCodeRect = SkRect::MakeLTRB(0, 33.0f, (SkScalar)this->Width, (SkScalar)this->Height);
    // ...
  }

  void MainWindow::onLeftMouseDown(SkScalar x, SkScalar y) {

    // Forward the event to a container control
    if (isPointInsideRect(x, y, tabCtrl.getRect()))
      tabCtrl.onLeftMouseDown(x, y);

    // [] Other controls' tests should go here
  }

  void MainWindow::onLeftMouseMove(SkScalar x, SkScalar y) {

    // Forward the event to a container control
    if (isPointInsideRect(x, y, tabCtrl.getRect()))
      tabCtrl.onLeftMouseMove(x, y);
    else if (tabCtrl.isTrackingActive() == true)
      tabCtrl.stopTracking();

    // [] Other controls' tests should go here
  }

  void MainWindow::onMouseLeave() {
    if (tabCtrl.isTrackingActive() == true)
      tabCtrl.stopTracking();
  }

  void MainWindow::onLeftMouseUp(SkScalar x, SkScalar y) {

    // Forward the event to a container control
    if (isPointInsideRect(x, y, tabCtrl.getRect()))
      tabCtrl.onLeftMouseUp(x, y);    

    // [] Other controls' tests should go here
  }

  void MainWindow::onKeyDown(VirtualKeycode key) {

    // TODO: every key event should be directed to the code edit control, except Ctrl+ and Alt+ augmented chords
    
    // DEBUG
    if (key == VirtualKeycode::VK_N)
      tabCtrl.addNewTab("New tab!");
  }

} // namespace varco
