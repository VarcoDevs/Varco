#include <UI/TabCtrl.hpp>

namespace varco {

  void TabCtrl::setRect(SkRect rect) {
    this->rect = rect;
  }

  void TabCtrl::paint(SkCanvas *canvas) {
    SkPaint background;
    background.setColor(SkColorSetRGB(39, 40, 34));
    canvas->drawRect(rect, background);
    SkPaint line;
    line.setColor(SkColorSetRGB(255, 0, 0));
    canvas->drawLine(rect.left(), rect.bottom() - 2, rect.right(), rect.top(), line);
  }

}
