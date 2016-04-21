#include <UI/CodeEditCtrl/CodeEditCtrl.hpp>
#include <SkCanvas.h>

namespace varco {

  CodeEditCtrl::CodeEditCtrl(MainWindow& parentWindow)
    : UICtrlBase(parentWindow)
  {}

  void CodeEditCtrl::paint() {

    if (!dirty)
      return;

    dirty = false; // It will be false at the end of this function, unless overridden

    SkCanvas canvas(this->bitmap);

    canvas.save();

    //////////////////////////////////////////////////////////////////////
    // Draw the background of the control
    //////////////////////////////////////////////////////////////////////
    {
      SkPaint background;
      background.setColor(SkColorSetARGB(255, 39, 40, 34));
      canvas.drawRect(this->rect, background);
    }

  }

}