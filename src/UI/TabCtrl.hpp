#ifndef VARCO_TABCTRL_HPP
#define VARCO_TABCTRL_HPP

#include "SkCanvas.h"
#include <string>
#include <list>

namespace varco {
  class Tab {
  public:
    Tab(std::string title);
  private:
    std::string title;
  };

  class TabCtrl {
  public:
    TabCtrl();

    // Set the new rect where the control will redraw itself
    void setRect(SkRect rect);
    // Paint the control on the given canvas
    void paint(SkCanvas *canvas);

  private:
    SkRect rect;
    std::list<Tab> tabs;
  };
}

#endif // VARCO_TABCTRL_HPP
