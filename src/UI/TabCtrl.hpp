#ifndef VARCO_TABCTRL_HPP
#define VARCO_TABCTRL_HPP

#include "SkCanvas.h"

namespace varco {
  class TabCtrl {
  public:
    void setRect(SkRect rect);
    void paint(SkCanvas *canvas);

  private:
    SkRect rect;
  };
}

#endif // VARCO_TABCTRL_HPP
