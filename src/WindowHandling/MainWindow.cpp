#include <WindowHandling/MainWindow.hpp>
#include <SkCanvas.h>
#include <Utils/Utils.hpp>


namespace varco {

#ifdef _WIN32
  MainWindow::MainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) : 
    BaseOSWindow(hInstance, hPrevInstance, lpCmdLine, nCmdShow),
#elif defined __linux__
  MainWindow::MainWindow(int argc, char **argv)
    : BaseOSWindow(argc, argv),
#endif
      m_tabCtrl(*this),
      m_codeEditCtrl(*this)
  {
    // DEBUG - Add some debug tabs
    int id = m_tabCtrl.addNewTab("SimpleFile.cpp");
    auto it = m_tabDocumentMap.emplace(id, Document(m_codeEditCtrl));
    std::pair<const int, Document>& element = *(it.first);
    element.second.loadFromFile("../TestData/SimpleFile.cpp");
    //it.first->second->applySyntaxHighlight(CPP);
    m_codeEditCtrl.loadDocument(element.second);
    
    //tabCtrl.addNewTab("Second tab");
    //tabCtrl.addNewTab("Third tab");
  }

  // Main window drawing entry point
  void MainWindow::draw(SkCanvas& canvas) {
    // Clear background color
    //canvas.drawColor(SkColorSetARGB(255, 39, 40, 34));

    // Calculate TabBar region
    SkRect tabCtrlRect = SkRect::MakeLTRB(0, 0, (SkScalar)this->Width, 33.0f);
    // Draw the TabBar region if needed
    m_tabCtrl.resize(tabCtrlRect);
    m_tabCtrl.paint();
    canvas.drawBitmap(m_tabCtrl.getBitmap(), m_tabCtrl.getRect().left(),
                      m_tabCtrl.getRect().top());

    // Calculate CodeView region
    SkRect codeEditCtrlRect = SkRect::MakeLTRB(0, 33.0f, (SkScalar)this->Width, (SkScalar)this->Height);
    // Draw the TabBar region if needed
    m_codeEditCtrl.resize(codeEditCtrlRect);
    m_codeEditCtrl.paint();
    canvas.drawBitmap(m_codeEditCtrl.getBitmap(), m_codeEditCtrl.getRect().left(), 
                      m_codeEditCtrl.getRect().top());
  }

  void MainWindow::onLeftMouseDown(SkScalar x, SkScalar y) {

    // Forward the event to a container control
    if (isPointInsideRect(x, y, m_tabCtrl.getRect()))
      m_tabCtrl.onLeftMouseDown(x, y);

    // [] Other controls' tests should go here
  }

  void MainWindow::onLeftMouseMove(SkScalar x, SkScalar y) {

    // Forward the event to a container control
    if (isPointInsideRect(x, y, m_tabCtrl.getRect()))
      m_tabCtrl.onLeftMouseMove(x, y);
    else if (m_tabCtrl.isTrackingActive() == true)
      m_tabCtrl.stopTracking();

    // [] Other controls' tests should go here
  }

  void MainWindow::onMouseLeave() {
    if (m_tabCtrl.isTrackingActive() == true)
      m_tabCtrl.stopTracking();
  }

  void MainWindow::onLeftMouseUp(SkScalar x, SkScalar y) {

    // Forward the event to a container control
    if (isPointInsideRect(x, y, m_tabCtrl.getRect()))
      m_tabCtrl.onLeftMouseUp(x, y);    

    // [] Other controls' tests should go here
  }

  void MainWindow::onKeyDown(VirtualKeycode key) {

    // TODO: every key event should be directed to the code edit control, except Ctrl+ and Alt+ augmented chords
    
    // DEBUG
    if (key == VirtualKeycode::VK_N)
      m_tabCtrl.addNewTab("New tab!");
  }

  void MainWindow::startMouseCapture() {
    BaseOSWindow::startMouseCapture();
  }

} // namespace varco
